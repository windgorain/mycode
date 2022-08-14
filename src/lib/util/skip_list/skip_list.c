/*================================================================
*   Created by LiXingang
*   Description: 跳表
*
================================================================*/
#include "bs.h"
#include "utl/rand_utl.h"
#include "utl/skip_list.h"

#define SKL_MAX_LEVEL 32

//定义结点
typedef struct SKL_NODE SKL_NODE_S;
typedef struct SKL_NODE {
    UINT key;
    UCHAR level;
    void *value;
    SKL_NODE_S *forward[1]; /* SKL_NODE_S */
}SKL_NODE_S;

//定义跳跃表
typedef struct {
    int max_level;
    UINT count;
    SKL_NODE_S *header;
}SKL_CTRL_S;

static int skl_rand_level()
{
    UINT rand = RAND_Get();
    int level = 0;
    int i;

    for (i=0; i<32; i++) {
        if (0 == (rand & (0x1<<i))) {
            break;
        }
        level ++;
    }

    return level;
}

static SKL_NODE_S * skl_new_node(int level)
{
    int total_size = sizeof(SKL_NODE_S) + level*sizeof(SKL_NODE_S*);
    SKL_NODE_S *node = MEM_ZMalloc(total_size);
    return node;
}

SKL_HDL SKL_Create(int max_level/* 包含最底层,需要最大多少级 */)
{
    BS_DBGASSERT(max_level <= SKL_MAX_LEVEL);
    BS_DBGASSERT(max_level >= 1);

    SKL_CTRL_S *ctrl = MEM_ZMalloc(sizeof(SKL_CTRL_S));
    if (! ctrl) {
        return NULL;
    }
    ctrl->header = skl_new_node(max_level-1);
    if (!ctrl->header) {
        MEM_Free(ctrl);
        return NULL;
    }

    ctrl->max_level = max_level;

    return ctrl;
}

void SKL_Reset(SKL_HDL hSkl)
{
    SKL_CTRL_S *ctrl = hSkl;
    SKL_NODE_S *x = ctrl->header;
    SKL_NODE_S *next;

    while ((next = x->forward[0])) {
        MEM_Free(x);
        x = next;
    }

    memset(ctrl->header, 0, sizeof(SKL_NODE_S) + (ctrl->max_level-1)*sizeof(SKL_NODE_S*));

    ctrl->count = 0;
}

void SKL_Destroy(SKL_HDL hSkl)
{
    SKL_CTRL_S *ctrl = hSkl;

    SKL_Reset(hSkl);

    if (ctrl->header) {
        MEM_Free(ctrl->header);
    }

    MEM_Free(hSkl);
}

void * SKL_GetFirst(SKL_HDL hSkl)
{
    SKL_CTRL_S *ctrl = hSkl;
    SKL_NODE_S *x = ctrl->header;

    if (x->forward[0]) {
        return x->forward[0]->value;
    }

    return NULL;
}

void * SKL_GetLast(SKL_HDL hSkl)
{
    return SKL_GetInLeft(hSkl, 0xffffffff);
}

/* 查询 <=key的最大节点 */
void * SKL_GetInLeft(SKL_HDL hSkl, UINT key)
{
    SKL_CTRL_S *ctrl = hSkl;
    int i;
    SKL_NODE_S *x = ctrl->header;

    for(i = ctrl->max_level-1; i >= 0; --i){
        while ((x->forward[i]) && (x->forward[i]->key < key)){
            x = x->forward[i];
        }
    }

    if ((x->forward[0]) && (x->forward[0]->key == key)) {
        return x->forward[0]->value;
    }

    return x->value;
}

/* 查询 >=key的最小节点 */
void * SKL_GetInRight(SKL_HDL hSkl, UINT key)
{
    SKL_CTRL_S *ctrl = hSkl;
    int i;
    SKL_NODE_S *x = ctrl->header;

    for(i = ctrl->max_level-1; i >= 0; --i){
        while ((x->forward[i]) && (x->forward[i]->key < key)){
            x = x->forward[i];
        }
    }

    x = x->forward[0];
    if (! x) {
        return NULL;
    }

    return x->value;
}

/* 查询 <key的最大节点 */
void * SKL_GetLeft(SKL_HDL hSkl, UINT key)
{
    SKL_CTRL_S *ctrl = hSkl;
    int i;
    SKL_NODE_S *x = ctrl->header;

    for(i = ctrl->max_level-1; i >= 0; --i){
        while ((x->forward[i]) && (x->forward[i]->key < key)){
            x = x->forward[i];
        }
    }

    return x->value;
}

/* 查询 >key的最小节点 */
void * SKL_GetRight(SKL_HDL hSkl, UINT key)
{
    SKL_CTRL_S *ctrl = hSkl;
    int i;
    SKL_NODE_S *x = ctrl->header;

    for(i = ctrl->max_level-1; i >= 0; --i){
        while ((x->forward[i]) && (x->forward[i]->key < key)){
            x = x->forward[i];
        }
    }

    x = x->forward[0];
    if (! x) {
        return NULL;
    }

    if (x->key == key) {
        x = x->forward[0];
    }

    if (! x) {
        return NULL;
    }

    return x->value;
}

void * SKL_Search(SKL_HDL hSkl, UINT key)
{
    SKL_CTRL_S *ctrl = hSkl;
    int i;
    SKL_NODE_S *x = ctrl->header;

    for(i = ctrl->max_level-1; i >= 0; --i){
        while ((x->forward[i]) && (x->forward[i]->key < key)){
            x = x->forward[i];
        }
    }

    x = x->forward[0];
    if (x && (x->key == key)) {
        return x->value;
    }

    return NULL;
}

int SKL_Insert(SKL_HDL hSkl, UINT key, void *value)
{
    SKL_CTRL_S *ctrl = hSkl;
    SKL_NODE_S *update[SKL_MAX_LEVEL];
    SKL_NODE_S *x = ctrl->header;
    int i;

    //寻找key所要插入的位置
    //保存大于key的位置信息
    for(i = ctrl->max_level-1; i >= 0; --i){
        while ((x->forward[i]) && (x->forward[i]->key < key)) {
            x = x->forward[i];
        }
        update[i] = x;
    }

    x = x->forward[0];
    //如果key已经存在
    if(x->key == key) {
        x->value = value;
        return BS_ALREADY_EXIST;
    }

    //随机生成新结点的层数
    int level = skl_rand_level();

    //申请新的结点
    SKL_NODE_S *new_node = skl_new_node(level);
    if (! new_node) {
        RETURN(BS_NO_MEMORY);
    }
    new_node->key = key;
    new_node->value = value;
    new_node->level = level;

    //调整forward指针
    for(i = level; i >= 0; --i){
        x = update[i];
        new_node->forward[i] = x->forward[i];
        x->forward[i] = new_node;
    }

    //更新元素数目
    ctrl->count ++;

    return 0;
}

void * SKL_Delete(SKL_HDL hSkl, UINT key)
{
    SKL_CTRL_S *ctrl = hSkl;
    SKL_NODE_S *update[SKL_MAX_LEVEL];
    SKL_NODE_S *x = ctrl->header;
    int i;

    //寻找要删除的结点
    for(i = ctrl->max_level-1; i >= 0; --i) {
        while(x->forward[i]->key < key){
            x = x->forward[i];
        }
        update[i] = x;
    }

    x = x->forward[0];
    if ((!x) || (x->key != key)) {
        return NULL;
    }

    void *value = x->value;

    //调整指针
    for (i = 0; i <= ctrl->max_level-1; ++i) {
        if(update[i]->forward[i] != x)
            break;
        update[i]->forward[i] = x->forward[i];
    }

    //删除结点
    MEM_Free(x);

    //更新链表元素数目
    ctrl->count --;

    return value;
}

void SKL_Print(SKL_HDL hSkl)
{
    SKL_CTRL_S *ctrl = hSkl;
    SKL_NODE_S *x = ctrl->header->forward[0];

    while (x) {
        printf("%12u %u", x->key, x->level);
    }
}

