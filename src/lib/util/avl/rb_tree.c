/*================================================================
*   Created by LiXingang
*   Description: 红黑树
*
================================================================*/
#include "bs.h"
#include "utl/rb_tree.h"

#define RB_PARENT(r)   ((r)->parent)
#define rb_color(r) ((r)->color)
#define rb_is_red(r)   ((r)->color==RB_TREE_RED)
#define rb_is_black(r)  ((r)->color==RB_TREE_BLACK)
#define rb_set_black(r)  do { (r)->color = RB_TREE_BLACK; } while (0)
#define rb_set_red(r)  do { (r)->color = RB_TREE_RED; } while (0)
#define rb_set_parent(r,p)  do { (r)->parent = (p); } while (0)
#define rb_set_color(r,c)  do { (r)->color = (c); } while (0)


static int _rbtree_preorder_walk(RB_TREE_NODE_S * tree, PF_RBTREE_WALK walk_func, void *ud)
{
    if (! tree) {
        return BS_WALK_CONTINUE;
    }

    if (BS_WALK_STOP == walk_func(tree, ud)) {
        return BS_WALK_STOP;
    }

    if (BS_WALK_STOP == _rbtree_preorder_walk(tree->left, walk_func, ud)) {
        return BS_WALK_STOP;
    }

    if (BS_WALK_STOP == _rbtree_preorder_walk(tree->right, walk_func, ud)) {
        return BS_WALK_STOP;
    }

    return BS_WALK_CONTINUE;
}

static int _rbtree_inorder_walk(RB_TREE_NODE_S * tree, PF_RBTREE_WALK walk_func, void *ud)
{
    if (! tree) {
        return BS_WALK_CONTINUE;
    }

    if (BS_WALK_STOP == _rbtree_inorder_walk(tree->left, walk_func, ud)) {
        return BS_WALK_STOP;
    }

    if (BS_WALK_STOP == walk_func(tree, ud)) {
        return BS_WALK_STOP;
    }

    if (BS_WALK_STOP == _rbtree_inorder_walk(tree->right, walk_func, ud)) {
        return BS_WALK_STOP;
    }

    return BS_WALK_CONTINUE;
}

static int _rbtree_postorder_walk(RB_TREE_NODE_S * tree, PF_RBTREE_WALK walk_func, void *ud)
{
    if (! tree) {
        return BS_WALK_CONTINUE;
    }

    if (BS_WALK_STOP == _rbtree_postorder_walk(tree->left, walk_func, ud)) {
        return BS_WALK_STOP;
    }

    if (BS_WALK_STOP == _rbtree_postorder_walk(tree->right, walk_func, ud)) {
        return BS_WALK_STOP;
    }

    if (BS_WALK_STOP == walk_func(tree, ud)) {
        return BS_WALK_STOP;
    }

    return BS_WALK_CONTINUE;
}

static inline RB_TREE_NODE_S * _rbtree_search(RB_TREE_NODE_S * x, void *key, PF_RBTREE_CMP cmp_func)
{
    int cmp_ret;

    while (x) {
        cmp_ret = cmp_func(key, x);
        if (cmp_ret == 0) {
            break;
        } else if (cmp_ret < 0) {
            x = x->left;
        } else {
            x = x->right;
        }
    }

    return x;
}

/*
 * 查找最小结点：返回tree为根结点的红黑树的最小结点。
 */
static inline RB_TREE_NODE_S * _rbtree_minimum(RB_TREE_NODE_S * tree)
{
    while(tree->left != NULL) {
        tree = tree->left;
    }

    return tree;
}

/*
 * 查找最大结点：返回tree为根结点的红黑树的最大结点。
 */
static inline RB_TREE_NODE_S * _rbtree_maximum(RB_TREE_NODE_S * tree)
{
    while(tree->right != NULL)
        tree = tree->right;
    return tree;
}

/*
 * 对红黑树的节点(x)进行左旋转
 *
 * 左旋示意图(对节点x进行左旋)：
 *      px                              px
 *     /                               /
 *    x                               y
 *   /  \      --(左旋)-->           / \
 *  lx   y                          x  ry
 *     /   \                       /  \
 *    ly   ry                     lx  ly
 *
 *
 */
static void _rbtree_left_rotate(RB_TREE_CTRL_S *root, RB_TREE_NODE_S *x)
{
    // 设置x的右孩子为y
    RB_TREE_NODE_S *y = x->right;

    // 将 “y的左孩子” 设为 “x的右孩子”；
    // 如果y的左孩子非空，将 “x” 设为 “y的左孩子的父亲”
    x->right = y->left;
    if (y->left != NULL)
        y->left->parent = x;

    // 将 “x的父亲” 设为 “y的父亲”
    y->parent = x->parent;

    if (x->parent == NULL)
    {
        //tree = y;            // 如果 “x的父亲” 是空节点，则将y设为根节点
        root->node = y;            // 如果 “x的父亲” 是空节点，则将y设为根节点
    }
    else
    {
        if (x->parent->left == x)
            x->parent->left = y;    // 如果 x是它父节点的左孩子，则将y设为“x的父节点的左孩子”
        else
            x->parent->right = y;    // 如果 x是它父节点的左孩子，则将y设为“x的父节点的左孩子”
    }

    // 将 “x” 设为 “y的左孩子”
    y->left = x;
    // 将 “x的父节点” 设为 “y”
    x->parent = y;
}

/*
 * 对红黑树的节点(y)进行右旋转
 *
 * 右旋示意图(对节点y进行左旋)：
 *            py                               py
 *           /                                /
 *          y                                x
 *         /  \      --(右旋)-->            /  \
 *        x   ry                           lx   y
 *       / \                                   / \
 *      lx  rx                                rx  ry
 *
 */
static void _rbtree_right_rotate(RB_TREE_CTRL_S *root, RB_TREE_NODE_S *y)
{
    // 设置x是当前节点的左孩子。
    RB_TREE_NODE_S *x = y->left;

    // 将 “x的右孩子” 设为 “y的左孩子”；
    // 如果"x的右孩子"不为空的话，将 “y” 设为 “x的右孩子的父亲”
    y->left = x->right;
    if (x->right != NULL)
        x->right->parent = y;

    // 将 “y的父亲” 设为 “x的父亲”
    x->parent = y->parent;

    if (y->parent == NULL)
    {
        //tree = x;            // 如果 “y的父亲” 是空节点，则将x设为根节点
        root->node = x;            // 如果 “y的父亲” 是空节点，则将x设为根节点
    }
    else
    {
        if (y == y->parent->right)
            y->parent->right = x;    // 如果 y是它父节点的右孩子，则将x设为“y的父节点的右孩子”
        else
            y->parent->left = x;    // (y是它父节点的左孩子) 将x设为“x的父节点的左孩子”
    }

    // 将 “y” 设为 “x的右孩子”
    x->right = y;

    // 将 “y的父节点” 设为 “x”
    y->parent = x;
}

/*
 * 红黑树插入修正函数
 *
 * 在向红黑树中插入节点之后(失去平衡)，再调用该函数；
 * 目的是将它重新塑造成一颗红黑树。
 *
 * 参数说明：
 *     root 红黑树的根
 *     node 插入的结点        // 对应《算法导论》中的z
 */
static void _rbtree_insert_fixup(RB_TREE_CTRL_S *root, RB_TREE_NODE_S *node)
{
    RB_TREE_NODE_S *parent, *gparent;

    // 若“父节点存在，并且父节点的颜色是红色”
    while ((parent = node->parent) && rb_is_red(parent)) {
        gparent = parent->parent;

        //若“父节点”是“祖父节点的左孩子”
        if (parent == gparent->left) {

            // Case 1条件：叔叔节点是红色
            RB_TREE_NODE_S *uncle = gparent->right;
            if (uncle && rb_is_red(uncle)) {
                rb_set_black(uncle);
                rb_set_black(parent);
                rb_set_red(gparent);
                node = gparent;
                continue;
            }

            // Case 2条件：叔叔是黑色，且当前节点是右孩子
            if (parent->right == node) {
                RB_TREE_NODE_S *tmp;
                _rbtree_left_rotate(root, parent);
                tmp = parent;
                parent = node;
                node = tmp;
            }

            // Case 3条件：叔叔是黑色，且当前节点是左孩子。
            rb_set_black(parent);
            rb_set_red(gparent);
            _rbtree_right_rotate(root, gparent);
        } else { //若“z的父节点”是“z的祖父节点的右孩子”

            // Case 1条件：叔叔节点是红色
            RB_TREE_NODE_S *uncle = gparent->left;
            if (uncle && rb_is_red(uncle))
            {
                rb_set_black(uncle);
                rb_set_black(parent);
                rb_set_red(gparent);
                node = gparent;
                continue;
            }

            // Case 2条件：叔叔是黑色，且当前节点是左孩子
            if (parent->left == node)
            {
                RB_TREE_NODE_S *tmp;
                _rbtree_right_rotate(root, parent);
                tmp = parent;
                parent = node;
                node = tmp;
            }

            // Case 3条件：叔叔是黑色，且当前节点是右孩子。
            rb_set_black(parent);
            rb_set_red(gparent);
            _rbtree_left_rotate(root, gparent);
        }
    }

    // 将根节点设为黑色
    rb_set_black(root->node);
}

/*
 * 添加节点：将节点(node)插入到红黑树中
 *
 * 参数说明：
 *     root 红黑树的根
 *     node 插入的结点        // 对应《算法导论》中的z
 */
static int _rbtree_insert(RB_TREE_CTRL_S *root, RB_TREE_NODE_S *node, void *key, PF_RBTREE_CMP cmp_func)
{
    RB_TREE_NODE_S *y = NULL;
    RB_TREE_NODE_S *x = root->node;
    int cmp_ret = 0, cmp_ret_y = 0;

    // 将红黑树当作一颗二叉查找树，将节点添加到二叉查找树中。
    while (x) {
        y = x;
        cmp_ret_y = cmp_ret;
        cmp_ret = cmp_func(key, x);
        if (cmp_ret < 0) {
            x = x->left;
        } else if (cmp_ret > 0) {
            x = x->right;
        } else {
            RETURN(BS_CONFLICT);
        }
    }

    node->parent = y;
    node->left = NULL;
    node->right = NULL;
    node->color = RB_TREE_RED;

    if (y != NULL) {
        if (cmp_ret_y < 0) {
            y->left = node; // node < y，将node设为“y的左孩子”
        } else {
            y->right = node; // node > y, 将node设为“y的右孩子”
        }
    } else {
        root->node = node; // y是空节点，则将node设为根
    }

    // 将它重新修正为一颗二叉查找树
    _rbtree_insert_fixup(root, node);

    return 0;
}

/*
 * 红黑树删除修正函数
 *
 * 在从红黑树中删除插入节点之后(红黑树失去平衡)，再调用该函数；
 * 目的是将它重新塑造成一颗红黑树。
 *
 * 参数说明：
 *     root 红黑树的根
 *     node 待修正的节点
 */
static void _rbtree_delete_fixup(RB_TREE_CTRL_S *root, RB_TREE_NODE_S *node, RB_TREE_NODE_S *parent)
{
    RB_TREE_NODE_S *other;

    while ((!node || rb_is_black(node)) && node != root->node)
    {
        if (parent->left == node)
        {
            other = parent->right;
            if (rb_is_red(other))
            {
                // Case 1: x的兄弟w是红色的
                rb_set_black(other);
                rb_set_red(parent);
                _rbtree_left_rotate(root, parent);
                other = parent->right;
            }
            if ((!other->left || rb_is_black(other->left)) &&
                (!other->right || rb_is_black(other->right)))
            {
                // Case 2: x的兄弟w是黑色，且w的俩个孩子也都是黑色的
                rb_set_red(other);
                node = parent;
                parent = RB_PARENT(node);
            }
            else
            {
                if (!other->right || rb_is_black(other->right))
                {
                    // Case 3: x的兄弟w是黑色的，并且w的左孩子是红色，右孩子为黑色。
                    rb_set_black(other->left);
                    rb_set_red(other);
                    _rbtree_right_rotate(root, other);
                    other = parent->right;
                }
                // Case 4: x的兄弟w是黑色的；并且w的右孩子是红色的，左孩子任意颜色。
                rb_set_color(other, rb_color(parent));
                rb_set_black(parent);
                rb_set_black(other->right);
                _rbtree_left_rotate(root, parent);
                node = root->node;
                break;
            }
        }
        else
        {
            other = parent->left;
            if (rb_is_red(other))
            {
                // Case 1: x的兄弟w是红色的
                rb_set_black(other);
                rb_set_red(parent);
                _rbtree_right_rotate(root, parent);
                other = parent->left;
            }
            if ((!other->left || rb_is_black(other->left)) &&
                (!other->right || rb_is_black(other->right)))
            {
                // Case 2: x的兄弟w是黑色，且w的俩个孩子也都是黑色的
                rb_set_red(other);
                node = parent;
                parent = RB_PARENT(node);
            }
            else
            {
                if (!other->left || rb_is_black(other->left))
                {
                    // Case 3: x的兄弟w是黑色的，并且w的左孩子是红色，右孩子为黑色。
                    rb_set_black(other->right);
                    rb_set_red(other);
                    _rbtree_left_rotate(root, other);
                    other = parent->left;
                }
                // Case 4: x的兄弟w是黑色的；并且w的右孩子是红色的，左孩子任意颜色。
                rb_set_color(other, rb_color(parent));
                rb_set_black(parent);
                rb_set_black(other->left);
                _rbtree_right_rotate(root, parent);
                node = root->node;
                break;
            }
        }
    }
    if (node)
        rb_set_black(node);
}

/*
 * 销毁红黑树
 */
static void _rbtree_destroy(RB_TREE_NODE_S * tree, PF_RBTREE_FREE free_func, void *ud)
{
    if (tree->left) {
        _rbtree_destroy(tree->left, free_func, ud);
    }

    if (tree->right) {
        _rbtree_destroy(tree->right, free_func, ud);
    }

    free_func(tree, ud);
}

/* 前序遍历"红黑树" */
void RBTree_PreOrderWalk(RB_TREE_CTRL_S *root, PF_RBTREE_WALK walk_func, void *ud)
{
    BS_DBGASSERT(NULL != root);
    _rbtree_preorder_walk(root->node, walk_func, ud);
}

/* 中序遍历"红黑树" */
void RBTree_InOrderWalk(RB_TREE_CTRL_S *root, PF_RBTREE_WALK walk_func, void *ud)
{
    BS_DBGASSERT(NULL != root);
    _rbtree_inorder_walk(root->node, walk_func, ud);
}

/* 后序遍历"红黑树" */
void RBTree_PostOrderWalk(RB_TREE_CTRL_S *root, PF_RBTREE_WALK walk_func, void *ud)
{
    BS_DBGASSERT(NULL != root);
    _rbtree_postorder_walk(root->node, walk_func, ud);
}

RB_TREE_NODE_S * RBTree_Search(RB_TREE_CTRL_S *root, void *key, PF_RBTREE_CMP cmp_func)
{
    BS_DBGASSERT(NULL != root);

    return _rbtree_search(root->node, key, cmp_func);
}

/* 获取比key大的最小的节点 */
RB_TREE_NODE_S * RBTree_SuccessorGet(RB_TREE_CTRL_S *root, void *key, PF_RBTREE_CMP cmp_func)
{
    RB_TREE_NODE_S *node = root->node;
    RB_TREE_NODE_S *superiorp = NULL;

    while (1) {
        int delta;  /* result of the comparison operation */

        if (node == NULL)
            return superiorp;

        delta = cmp_func(key, node);
        if (delta < 0) {
            superiorp = node; /* update superiorp */
            node = node->left;
        } else {
            node = node->right;
        }
    }
}

/* 获取比key小的最大的节点 */
RB_TREE_NODE_S * RBTree_PredecessorGet(RB_TREE_CTRL_S *root, void *key, PF_RBTREE_CMP cmp_func)
{
    RB_TREE_NODE_S *node = root->node;
    RB_TREE_NODE_S *inferiorp = NULL;

    while (1) {
        int delta;  /* result of the comparison operation */

        if (node == NULL)
            return inferiorp;

        delta = cmp_func(key, node);
        if (delta > 0) {
            inferiorp = node;
            node = node->right;
        } else {
            node = node->left;
        }
    }
}

RB_TREE_NODE_S * RBTree_Min(RB_TREE_CTRL_S *root)
{
    BS_DBGASSERT(root != NULL);

    if (! root->node) {
        return NULL;
    }

    return _rbtree_minimum(root->node);
}

RB_TREE_NODE_S * RBTree_Max(RB_TREE_CTRL_S *root)
{
    BS_DBGASSERT(NULL != root);

    if (! root->node) {
        return NULL;
    }

    return _rbtree_maximum(root->node);
}

/*
 * 找结点(x)的后继结点。即，查找"红黑树中数据值大于该结点"的"最小结点"。
 */
RB_TREE_NODE_S* RBTree_Next(RB_TREE_NODE_S * x)
{
    // 如果x存在右孩子，则"x的后继结点"为 "以其右孩子为根的子树的最小结点"。
    if (x->right != NULL) {
        return _rbtree_minimum(x->right);
    }

    // 如果x没有右孩子。则x有以下两种可能：
    // (01) x是"一个左孩子"，则"x的后继结点"为 "它的父结点"。
    // (02) x是"一个右孩子"，则查找"x的最低的父结点，并且该父结点要具有左孩子"，找到的这个"最低的父结点"就是"x的后继结点"。
    RB_TREE_NODE_S* y = x->parent;
    while ((y!=NULL) && (x==y->right)) {
        x = y;
        y = y->parent;
    }

    return y;
}

/*
 * 找结点(x)的前驱结点。即，查找"红黑树中数据值小于该结点"的"最大结点"。
 */
RB_TREE_NODE_S* RBTree_Pre(RB_TREE_NODE_S * x)
{
    // 如果x存在左孩子，则"x的前驱结点"为 "以其左孩子为根的子树的最大结点"。
    if (x->left != NULL) {
        return _rbtree_maximum(x->left);
    }

    // 如果x没有左孩子。则x有以下两种可能：
    // (01) x是"一个右孩子"，则"x的前驱结点"为 "它的父结点"。
    // (01) x是"一个左孩子"，则查找"x的最低的父结点，并且该父结点要具有右孩子"，找到的这个"最低的父结点"就是"x的前驱结点"。
    RB_TREE_NODE_S* y = x->parent;
    while ((y!=NULL) && (x==y->left)) {
        x = y;
        y = y->parent;
    }

    return y;
}

int RBTree_Insert(RB_TREE_CTRL_S *root, RB_TREE_NODE_S *node, void *key, PF_RBTREE_CMP cmp_func)
{
    return _rbtree_insert(root, node, key, cmp_func);
}

void RBTree_DelNode(RB_TREE_CTRL_S *root, RB_TREE_NODE_S *node)
{
    RB_TREE_NODE_S *child, *parent;
    int color;

    // 被删除节点的"左右孩子都不为空"的情况。
    if ( (node->left!=NULL) && (node->right!=NULL) ) {
        // 被删节点的后继节点。(称为"取代节点")
        // 用它来取代"被删节点"的位置，然后再将"被删节点"去掉。
        RB_TREE_NODE_S *replace = node;

        // 获取后继节点
        replace = replace->right;
        while (replace->left != NULL) {
            replace = replace->left;
        }

        // "node节点"不是根节点(只有根节点不存在父节点)
        if (node->parent) {
            if (node->parent->left == node)
                node->parent->left = replace;
            else
                node->parent->right = replace;
        } else {
            // "node节点"是根节点，更新根节点。
            root->node = replace;
        }

        // child是"取代节点"的右孩子，也是需要"调整的节点"。
        // "取代节点"肯定不存在左孩子！因为它是一个后继节点。
        child = replace->right;
        parent = replace->parent;
        // 保存"取代节点"的颜色
        color = rb_color(replace);

        // "被删除节点"是"它的后继节点的父节点"
        if (parent == node) {
            parent = replace;
        } else {
            // child不为空
            if (child) {
                rb_set_parent(child, parent);
            }
            parent->left = child;

            replace->right = node->right;
            rb_set_parent(node->right, replace);
        }

        replace->parent = node->parent;
        replace->color = node->color;
        replace->left = node->left;
        node->left->parent = replace;

        if (color == RB_TREE_BLACK) {
            _rbtree_delete_fixup(root, child, parent);
        }

        return;
    }

    if (node->left !=NULL) {
        child = node->left;
    } else {
        child = node->right;
    }

    parent = node->parent;
    // 保存"取代节点"的颜色
    color = node->color;

    if (child) {
        child->parent = parent;
    }

    // "node节点"不是根节点
    if (parent) {
        if (parent->left == node) {
            parent->left = child;
        } else {
            parent->right = child;
        }
    }
    else
        root->node = child;

    if (color == RB_TREE_BLACK) {
        _rbtree_delete_fixup(root, child, parent);
    }
}

RB_TREE_NODE_S * RBTree_Del(RB_TREE_CTRL_S *root, void *key, PF_RBTREE_CMP cmp_func)
{
    RB_TREE_NODE_S *z;

    z = _rbtree_search(root->node, key, cmp_func);

    if (z) {
        RBTree_DelNode(root, z);
    }

    return z;
}


void RBTree_Init(IN RB_TREE_CTRL_S *root)
{
    memset(root, 0, sizeof(RB_TREE_CTRL_S));
}

void RBTree_Finit(RB_TREE_CTRL_S *root, PF_RBTREE_FREE free_func, void *ud)
{
    if (! root) 
        return;

    if (root->node) {
        _rbtree_destroy(root->node, free_func, ud);
    }

    memset(root, 0, sizeof(RB_TREE_CTRL_S));
}


