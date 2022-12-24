/*================================================================
*   Created by LiXingang
*   Description:红黑树 
*
================================================================*/
#ifndef _RB_TREE_H
#define _RB_TREE_H
#ifdef __cplusplus
extern "C"
{
#endif

#define RB_TREE_RED      0    // 红色节点
#define RB_TREE_BLACK    1    // 黑色节点

// 红黑树的节点
typedef struct RBTreeNode {
    unsigned char color;        // 颜色(RB_TREE_RED 或 RB_TREE_BLACK)
    struct RBTreeNode *left;    // 左孩子
    struct RBTreeNode *right;    // 右孩子
    struct RBTreeNode *parent;    // 父结点
}RB_TREE_NODE_S;

// 红黑树的根
typedef struct rb_root{
    RB_TREE_NODE_S *node;
}RB_TREE_CTRL_S;

typedef int (*PF_RBTREE_CMP)(void *key, RB_TREE_NODE_S *node);
typedef BS_WALK_RET_E (*PF_RBTREE_WALK)(RB_TREE_NODE_S *node, void *ud);
typedef void (*PF_RBTREE_FREE)(RB_TREE_NODE_S *node, void *ud);

void RBTree_Init(IN RB_TREE_CTRL_S *root);
void RBTree_Finit(RB_TREE_CTRL_S *root, PF_RBTREE_FREE free_func, void *ud);
int RBTree_Insert(RB_TREE_CTRL_S *root, RB_TREE_NODE_S *node, void *key, PF_RBTREE_CMP cmp_func);
RB_TREE_NODE_S * RBTree_Search(RB_TREE_CTRL_S *root, void *key, PF_RBTREE_CMP cmp_func);
/* 获取比key大的最小的节点 */
RB_TREE_NODE_S * RBTree_SuccessorGet(RB_TREE_CTRL_S *root, void *key, PF_RBTREE_CMP cmp_func);
/* 获取比key小的最大的节点 */
RB_TREE_NODE_S * RBTree_PredecessorGet(RB_TREE_CTRL_S *root, void *key, PF_RBTREE_CMP cmp_func);
RB_TREE_NODE_S * RBTree_Del(RB_TREE_CTRL_S *root, void *key, PF_RBTREE_CMP cmp_func);
void RBTree_DelNode(RB_TREE_CTRL_S *root, RB_TREE_NODE_S *node);
RB_TREE_NODE_S * RBTree_Min(RB_TREE_CTRL_S *root);
RB_TREE_NODE_S * RBTree_Max(RB_TREE_CTRL_S *root);
// 前序遍历"红黑树"
void RBTree_PreOrderWalk(RB_TREE_CTRL_S *root, PF_RBTREE_WALK walk_func, void *ud);
// 中序遍历"红黑树"
void RBTree_InOrderWalk(RB_TREE_CTRL_S *root, PF_RBTREE_WALK walk_func, void *ud);
// 后序遍历"红黑树"
void RBTree_PostOrderWalk(RB_TREE_CTRL_S *root, PF_RBTREE_WALK walk_func, void *ud);
/* 找结点(x)的后继结点。即，查找"红黑树中数据值大于该结点"的"最小结点" */
RB_TREE_NODE_S* RBTree_Next(RB_TREE_NODE_S * x);
/* 找结点(x)的前驱结点。即，查找"红黑树中数据值小于该结点"的"最大结点" */
RB_TREE_NODE_S* RBTree_Pre(RB_TREE_NODE_S * x);

#ifdef __cplusplus
}
#endif
#endif //RB_TREE_H_
