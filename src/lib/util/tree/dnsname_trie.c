/*================================================================
*   Created by LiXingang：2018.11.07
*   Description：域名字典树, 用于查找/匹配 *.baidu.com之类的域名
*
================================================================*/
#include "bs.h"
#include "utl/txt_utl.h"
#include "utl/mem_utl.h"
#include "utl/dnsname_trie.h"


#define DNSNAME_TRIE_NODE_FLAG_MATCHED          0x1 /* 匹配节点*/
#define DNSNAME_TRIE_NODE_FLAG_WILDCARD_NEXT    0x2 /* 通配接下来的字符 */

static int dnsnametrie_GetIndex(char c)
{
    return (unsigned char)c;
}

void DnsNameTrie_Init(IN DNSNAME_TRIE_CTRL_S *ctrl, IN UINT flag)
{
    Mem_Zero(ctrl, sizeof(DNSNAME_TRIE_CTRL_S));
    ctrl->flag = flag;
}

static void dnsnametrie_Free(IN DNSNAME_TRIE_NODE_S *node, IN PF_DNSNAME_TRIE_USER_DATA_FREE pfUserDataFree)
{
    int i;

    for (i=0; i<DNSNAME_TRIE_CHAR_NUM; i++) {
        if (node->nexts[i] != NULL) {
            dnsnametrie_Free(node->nexts[i], pfUserDataFree);
        }
    }

    if (pfUserDataFree) {
        pfUserDataFree(node->user_data);
    }

    MEM_Free(node);
}

void DnsNameTrie_Fini(IN DNSNAME_TRIE_CTRL_S *ctrl, IN PF_DNSNAME_TRIE_USER_DATA_FREE pfUserDataFree)
{
    int i;
    DNSNAME_TRIE_NODE_S *root = &ctrl->root;

    for (i=0; i<DNSNAME_TRIE_CHAR_NUM; i++) {
        if (root->nexts[i] != NULL) {
            dnsnametrie_Free(root->nexts[i], pfUserDataFree);
            root->nexts[i] = NULL;
        }
    }
}

static void * dnsnametrie_DefaultSetUserData(void *user_data, void * user_handle)
{
    return user_handle;
}

int DnsNameTrie_InsertExt(DNSNAME_TRIE_CTRL_S *ctrl, char *hostname, PF_DNSNAME_TRIE_USER_DATA_SET pfSetUserData, void * user_handle)
{
    DNSNAME_TRIE_NODE_S *root = &ctrl->root;
    int i;
    int len;
    char c;
    int index;
    unsigned int flag = DNSNAME_TRIE_NODE_FLAG_MATCHED;
    DNSNAME_TRIE_NODE_S *cur;
    char invert_hostname[512];

    if (hostname[0] == '\0') {
        RETURN(BS_BAD_PARA);
    }

    len = strlen(hostname);
    if (len >= sizeof(invert_hostname)) {
        RETURN(BS_OUT_OF_RANGE);
    }

    MEM_Invert(hostname, len, invert_hostname);
    invert_hostname[len] = '\0';

    cur=root;
    for(i=0; invert_hostname[i] != '\0'; i++) {
        c = invert_hostname[i];
        if (c == '*') {
            flag = DNSNAME_TRIE_NODE_FLAG_WILDCARD_NEXT | DNSNAME_TRIE_NODE_FLAG_MATCHED;
            break;
        }

        index = dnsnametrie_GetIndex(c);
        if (index < 0) {
            RETURN(BS_OUT_OF_RANGE);
        }

        if(cur->nexts[index] == NULL) {
            DNSNAME_TRIE_NODE_S *newNode = (void*)MEM_ZMalloc(sizeof(DNSNAME_TRIE_NODE_S));
            if (newNode == NULL) {
                RETURN(BS_NO_MEMORY);
            }
            cur->nexts[index] = newNode;
        }
        cur = cur->nexts[index];
    }

    cur->flag &= (~DNSNAME_TRIE_NODE_FLAG_MATCHED);

    cur->user_data = pfSetUserData(cur->user_data, user_handle);

    if (cur->user_data != NULL) {
        cur->flag |= flag;
    }

    return 0;
}

int DnsNameTrie_Insert(DNSNAME_TRIE_CTRL_S *ctrl, char *hostname, void *user_data)
{
    BS_DBGASSERT(user_data != NULL);

    return DnsNameTrie_InsertExt(ctrl, hostname, dnsnametrie_DefaultSetUserData, user_data);
}

static BOOL_T dnsnametrie_Wildcard(IN DNSNAME_TRIE_CTRL_S *ctrl, IN DNSNAME_TRIE_NODE_S *node, IN char *string)
{
    if (! (node->flag & DNSNAME_TRIE_NODE_FLAG_WILDCARD_NEXT)) {
        return FALSE;
    }

    if ((ctrl->flag & DNSNAME_TRIE_FLAG_WILDCARD_ONE_LEVEL) && (strchr(string, '.') != NULL)) {
        return FALSE;
    }

    return TRUE;
}

/* 最长匹配域名,返回匹配的节点 */
DNSNAME_TRIE_NODE_S * DnsNameTrie_MatchNode(DNSNAME_TRIE_CTRL_S *ctrl, char *hostname, UINT len)
{
    DNSNAME_TRIE_NODE_S *root = &ctrl->root;
    char *read;
    int index;
    DNSNAME_TRIE_NODE_S *found;
    DNSNAME_TRIE_NODE_S *wildcard = NULL;
    char invert_hostname[512];

    if (hostname[0] == '\0') {
        return NULL;
    }

    if (len >= sizeof(invert_hostname)) {
        return NULL;
    }

    MEM_Invert(hostname, len, invert_hostname);
    invert_hostname[len] = '\0';

    found = root;
    read = invert_hostname;

    if (dnsnametrie_Wildcard(ctrl, found, read + 1)) {
        wildcard = found;
    }

    while (*read != '\0') {
        index = dnsnametrie_GetIndex(*read);

        if ((index < 0) || (found->nexts[index] == NULL)){
            found = NULL;
            break;
        }

        found = found->nexts[index];
        if (dnsnametrie_Wildcard(ctrl, found, read + 1)) {
            wildcard = found;
        }

        read ++;
    }

    if (found && (found->flag & DNSNAME_TRIE_NODE_FLAG_MATCHED)) {
        return found;
    }

    return wildcard;
}

/* 最长匹配域名,返回匹配的节点中的用户数据 */
void * DnsNameTrie_Match(DNSNAME_TRIE_CTRL_S *ctrl, char *hostname, UINT len)
{
    DNSNAME_TRIE_NODE_S *node;

    node = DnsNameTrie_MatchNode(ctrl, hostname, len);
    if (node == NULL) {
        return NULL;
    }

    return node->user_data;
}

static void dnsnametrie_MergeSubTreeUserData(DNSNAME_TRIE_NODE_S *node, PF_DNSNAME_TRIE_MERGE_WILDCARD_SUB pfFunc, void *user_data)
{
    int i;
    void * user_data_tmp;

    for (i=0; i<DNSNAME_TRIE_CHAR_NUM; i++) {
        if (node->nexts[i] == NULL) {
            continue;
        }

        if (node->flag & DNSNAME_TRIE_NODE_FLAG_MATCHED) {
            node->user_data = pfFunc(node->user_data, user_data);
        }

        if (node->flag & DNSNAME_TRIE_NODE_FLAG_WILDCARD_NEXT) {
            user_data_tmp = node->user_data;
        } else {
            user_data_tmp = user_data;
        }
        
        dnsnametrie_MergeSubTreeUserData(node, pfFunc, user_data_tmp);
    }
}

/* 遍历合并每个通配子集的user_data */
void DnsNameTrie_MergeSubTreeUserData(DNSNAME_TRIE_CTRL_S *ctrl, PF_DNSNAME_TRIE_MERGE_WILDCARD_SUB pfFunc)
{
    DNSNAME_TRIE_NODE_S *node = &ctrl->root;
    void *user_data = NULL;

    if (node->flag & DNSNAME_TRIE_NODE_FLAG_WILDCARD_NEXT) {
        user_data = node->user_data;
    }

    dnsnametrie_MergeSubTreeUserData(node, pfFunc, user_data);
}

int DnsNameTrie_Del(DNSNAME_TRIE_CTRL_S *ctrl, char *hostname, PF_DNSNAME_TRIE_USER_DATA_FREE free_func)
{
    if (hostname == NULL) return 0;

    if (hostname[0] == '*' && strlen(hostname) == 1) {
        DnsNameTrie_Fini(ctrl, free_func);
        return 0;
    }

    DNSNAME_TRIE_NODE_S *node = DnsNameTrie_MatchNode(ctrl, hostname, strlen(hostname));
    if (node) {
        dnsnametrie_Free(node, free_func);
        return 0;
    }

    return -1;
}
