/* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_AVL

#include "bs.h"

#include "utl/avllib_utl.h"

/* defines */

#define AVL_MAX_HEIGHT 42 /* The meaning of life, the universe and everything.
                 Plus, the nodes for a tree this high would use
                 more than 2**32 bytes anyway */

/*******************************************************************************
*
* avlRebalance - make sure the tree conforms to the AVL balancing rules, while
* preserving the ordering of the binary tree
*
* INTERNAL
* The AVL tree balancing rules are as follows :
* - the height of the left and right subtrees under a given node must never
*   differ by more than one
* - the height of a given subtree is defined as 1 plus the maximum height of
*   the subtrees under his root node
*
* The avlRebalance procedure must be called after a leaf node has been inserted
* or deleted from the tree. It checks that the AVL balancing rules are
* respected, makes local adjustments to the tree if necessary, recalculates
* the height field of the modified nodes, and repeats the process for every
* node up to the root node. This iteration is necessary because the balancing
* rules for a given node might have been broken by the modification we did on
* one of the subtrees under it.
*
* Because we need to iterate the process up to the root node, and the tree
* nodes does not contain poLONGers to their father node, we ask the caller of
* this procedure to keep a list of all the nodes traversed from the root node
* to the node just before the recently inserted or deleted node. This is the
* <ancestors> argument. Because each subtree might have to be re-rooted in the
* balancing operation, <ancestors> is actually a list poLONGers to the node
* poLONGers - thus if re-rooting occurs, the node poLONGers can be modified so
* that they keep poLONGing to the root of a given subtree.
*
* <count> is simply a count of elements in the <ancestors> list.
*
* RETURNS: N/A
*/

void avlRebalance
(
    AVL_NODE ***    ancestors,  /* list of poLONGers to the ancestor  node poLONGers */
    int         count       /* number ancestors to rebalance */
)
{
    while (count > 0)
    {
    AVL_NODE ** nodepp; /* address of the poLONGer to the root node of
                   the current subtree */
    AVL_NODE *  nodep;  /* poLONGs to root node of current subtree */
    AVL_NODE *  leftp;  /* poLONGs to root node of left subtree */
    int     lefth;  /* height of the left subtree */
    AVL_NODE *  rightp; /* poLONGs to root node of right subtree */
    int     righth; /* height of the right subtree */

    /* 
     * Find the current root node and its two subtrees. By construction,
     * we know that both of them conform to the AVL balancing rules.
     */

    nodepp = ancestors[--count];
    nodep = *nodepp;
    leftp = nodep->left;
    lefth = (leftp != NULL) ? leftp->height : 0;
    rightp = nodep->right;
    righth = (rightp != NULL) ? rightp->height : 0;

    if (righth - lefth < -1)
    {
        /*
         *         *
         *       /   \
         *    n+2      n
         *
         * The current subtree violates the balancing rules by beeing too
         * high on the left side. We must use one of two different
         * rebalancing methods depending on the configuration of the left
         * subtree.
         *
         * Note that leftp cannot be NULL or we would not pass there !
         */

        AVL_NODE *  leftleftp;  /* poLONGs to root of left left
                       subtree */
        AVL_NODE *  leftrightp; /* poLONGs to root of left right
                       subtree */
        int     leftrighth; /* height of left right subtree */

        leftleftp = leftp->left;
        leftrightp = leftp->right;
        leftrighth = (leftrightp != NULL) ? leftrightp->height : 0;

        if ((leftleftp != NULL) && (leftleftp->height >= leftrighth))
        {
        /*
         *            <D>                     <B>
         *             *                    n+2|n+3
         *           /   \                   /   \
         *        <B>     <E>    ---->    <A>     <D>
         *        n+2      n              n+1   n+1|n+2
         *       /   \                           /   \
         *    <A>     <C>                     <C>     <E>
         *    n+1    n|n+1                   n|n+1     n
         */

        nodep->left = leftrightp;   /* D.left = C */
        nodep->height = leftrighth + 1;
        leftp->right = nodep;       /* B.right = D */
        leftp->height = leftrighth + 2;
        *nodepp = leftp;        /* B becomes root */
        }
        else
        {
        /*
         *           <F>
         *            *
         *          /   \                        <D>
         *       <B>     <G>                     n+2
         *       n+2      n                     /   \
         *      /   \           ---->        <B>     <F>
         *   <A>     <D>                     n+1     n+1
         *    n      n+1                    /  \     /  \
         *          /   \                <A>   <C> <E>   <G>
         *       <C>     <E>              n  n|n-1 n|n-1  n
         *      n|n-1   n|n-1
         *
         * We can assume that leftrightp is not NULL because we expect
         * leftp and rightp to conform to the AVL balancing rules.
         * Note that if this assumption is wrong, the algorithm will
         * crash here.
         */

        leftp->right = leftrightp->left;    /* B.right = C */
        leftp->height = leftrighth;
        nodep->left = leftrightp->right;    /* F.left = E */
        nodep->height = leftrighth;
        leftrightp->left = leftp;       /* D.left = B */
        leftrightp->right = nodep;      /* D.right = F */
        leftrightp->height = leftrighth + 1;
        *nodepp = leftrightp;           /* D becomes root */
        }
        }
    else if (righth - lefth > 1)
    {
        /*
         *        *
         *      /   \
         *    n      n+2
         *
         * The current subtree violates the balancing rules by beeing too
         * high on the right side. This is exactly symmetric to the
         * previous case. We must use one of two different rebalancing
         * methods depending on the configuration of the right subtree.
         *
         * Note that rightp cannot be NULL or we would not pass there !
         */

        AVL_NODE *  rightleftp; /* poLONGs to the root of right left
                       subtree */
        int     rightlefth; /* height of right left subtree */
        AVL_NODE *  rightrightp;    /* poLONGs to the root of right right
                       subtree */

        rightleftp = rightp->left;
        rightlefth = (rightleftp != NULL) ? rightleftp->height : 0;
        rightrightp = rightp->right;

        if ((rightrightp != NULL) && (rightrightp->height >= rightlefth))
        {
        /*        <B>                             <D>
         *         *                            n+2|n+3
         *       /   \                           /   \
         *    <A>     <D>        ---->        <B>     <E>
         *     n      n+2                   n+1|n+2   n+1
         *           /   \                   /   \
         *        <C>     <E>             <A>     <C>
         *       n|n+1    n+1              n     n|n+1
         */

        nodep->right = rightleftp;  /* B.right = C */
        nodep->height = rightlefth + 1;
        rightp->left = nodep;       /* D.left = B */
        rightp->height = rightlefth + 2;
        *nodepp = rightp;       /* D becomes root */
        }
        else
        {
        /*        <B>
         *         *
         *       /   \                            <D>
         *    <A>     <F>                         n+2
         *     n      n+2                        /   \
         *           /   \       ---->        <B>     <F>
         *        <D>     <G>                 n+1     n+1
         *        n+1      n                 /  \     /  \
         *       /   \                    <A>   <C> <E>   <G>
         *    <C>     <E>                  n  n|n-1 n|n-1  n
         *   n|n-1   n|n-1
         *
         * We can assume that rightleftp is not NULL because we expect
         * leftp and rightp to conform to the AVL balancing rules.
         * Note that if this assumption is wrong, the algorithm will
         * crash here.
         */

        nodep->right = rightleftp->left;    /* B.right = C */
        nodep->height = rightlefth;
        rightp->left = rightleftp->right;   /* F.left = E */
        rightp->height = rightlefth;
        rightleftp->left = nodep;       /* D.left = B */
        rightleftp->right = rightp;     /* D.right = F */
        rightleftp->height = rightlefth + 1;
        *nodepp = rightleftp;           /* D becomes root */
        }
        }
    else
    {
        /*
         * No rebalancing, just set the tree height
         *
         * If the height of the current subtree has not changed, we can
         * stop here because we know that we have not broken the AVL
         * balancing rules for our ancestors.
         */

        int height;

        height = ((righth > lefth) ? righth : lefth) + 1;
        if (nodep->height == height)
        break;
        nodep->height = height;
        }
    }
}

/*******************************************************************************
*
* avlSearch - search a node in an AVL tree
*
* At the time of the call, <root> is the root node poLONGer. <key> is the value
* we want to search, and <compare> is the user-provided comparison function.
*
* Note that we cannot have several nodes with the equal keys in the tree, so
* there is no ambiguity about which node will be found.
*
* Also note that the search procedure does not depend on the tree balancing
* rules, but because the tree is balanced, we know that the search procedure
* will always be efficient.
*
* RETURNS: poLONGer to the node whose key equals <key>, or NULL if there is
* no such node in the tree
*/

void * avlSearch(AVL_TREE root, void *key, PF_AVL_CMP_FUNC cmp_func)
{
    AVL_NODE *  nodep;  /* poLONGer to the current node */

    nodep = root;
    while (1)
    {
        int delta;  /* result of the comparison operation */

        if (nodep == NULL)
            return NULL;    /* not found ! */

        delta = cmp_func(key, nodep);
        if (0 == delta)
            return nodep;   /* found the node */
        else if (delta < 0)
            nodep = nodep->left;
        else
            nodep = nodep->right;
    }
}

/*******************************************************************************
 *
 * avlSearchUnsigned - search a node in an AVL tree
 *
 * This is a specialized implementation of avlSearch for cases where the
 * node to be searched is an AVL_UNSIGNED_NODE.
 *
 * RETURNS: poLONGer to the node whose key equals <key>, or NULL if there is
 * no such node in the tree
 */

void * avlSearchUnsigned(AVL_TREE root, UINT key)
{
    AVL_UNSIGNED_NODE * nodep;  /* poLONGer to the current node */

    nodep = (AVL_UNSIGNED_NODE *) root;
    while (1)
    {
        if (nodep == NULL)
            return NULL;    /* not found ! */

        if (key == nodep->key)
            return nodep;   /* found the node */
        else if (key < nodep->key)
            nodep = nodep->avl.left;
        else
            nodep = nodep->avl.right;
    }
}

/*******************************************************************************
 *
 * avlInsert - insert a node in an AVL tree
 *
 * At the time of the call, <root> poLONGs to the root node poLONGer. This root
 * node poLONGer is possibly NULL if the tree is empty. <newNode> poLONGs to the
 * node we want to insert. His left, right and height fields need not be filled,
 * but the user will probably have added his own data fields after those. <key>
 * is newNode's key, that will be passed to the comparison function. This is
 * redundant because it could really be derived from newNode, but the way to do
 * this depends on the precise type of newNode so we cannot do this in a generic
 * routine. <compare> is the user-provided comparison function.
 *
* Note that we cannot have several nodes with the equal keys in the tree, so
* the insertion operation will fail if we try to insert a node that has a
* duplicate key.
*
* Also note that because we keep the tree balanced, the root node poLONGer that
* is poLONGed by the <root> argument can be modified in this function.
*
* INTERNAL
* The insertion routine works just like in a non-balanced binary tree : we
* walk down the tree like if we were searching a node, and when we reach a leaf
* node we insert newNode at this position.
*
* Because the balancing procedure needs to be able to walk back to the root
* node, we keep a list of poLONGers to the poLONGers we followed on our way down
* the tree.
*
* RETURNS: BS_OK, or BS_ERR if the tree already contained a node with the same key
*/

int avlInsert (AVL_TREE *root, void *newNode, void *key, PF_AVL_CMP_FUNC cmp_func)
{
    AVL_NODE ** nodepp;             /* ptr to current node ptr */
    AVL_NODE ** ancestor[AVL_MAX_HEIGHT];   /* list of poLONGers to all
                                               our ancestor node ptrs */
    int ancestorCount;          /* number of ancestors */

    nodepp = root;
    ancestorCount = 0;

    while (1) {
        AVL_NODE *  nodep;  /* poLONGer to the current node */
        int     delta;  /* result of the comparison operation */

        nodep = *nodepp;
        if (nodep == NULL)
            break;  /* we can insert a leaf node here ! */

        ancestor[ancestorCount++] = nodepp;

        delta = cmp_func(key, nodep);
        if (0 == delta)
            RETURN(BS_ALREADY_EXIST);
        else if (delta < 0)
            nodepp = (AVL_NODE **)&(nodep->left);
        else
            nodepp = (AVL_NODE **)&(nodep->right);
    }

    ((AVL_NODE *)newNode)->left = NULL;
    ((AVL_NODE *)newNode)->right = NULL;
    ((AVL_NODE *)newNode)->height = 1;
    *nodepp = newNode;

    avlRebalance (ancestor, ancestorCount);

    return BS_OK;
}

/*******************************************************************************
 *
 * avlInsertUnsigned - insert a node in an AVL tree
 *
 * This is a specialized implementation of avlInsert for cases where the
 * node to be inserted is an AVL_UNSIGNED_NODE.
 *
 * RETURNS: BS_OK, or BS_ERR if the tree already contained a node with the same key
 */

int avlInsertUnsigned(AVL_TREE *root, void *newNode)
{
    AVL_UNSIGNED_NODE **nodepp; /* ptr to current node ptr */
    AVL_UNSIGNED_NODE **ancestor[AVL_MAX_HEIGHT];
            /* list of poLONGers to all our ancestor node ptrs */
    int ancestorCount;      /* number of ancestors */
    UINT key;

    key = ((AVL_UNSIGNED_NODE *)newNode)->key;
    nodepp = (AVL_UNSIGNED_NODE **) root;
    ancestorCount = 0;

    while (1) {
        AVL_UNSIGNED_NODE * nodep;  /* poLONGer to the current node */

        nodep = *nodepp;
        if (nodep == NULL)
            break;  /* we can insert a leaf node here ! */

        ancestor[ancestorCount++] = nodepp;

        if (key == nodep->key)
            RETURN(BS_ERR);
        else if (key < nodep->key)
            nodepp = (AVL_UNSIGNED_NODE **)&(nodep->avl.left);
        else
            nodepp = (AVL_UNSIGNED_NODE **)&(nodep->avl.right);
    }

    ((AVL_NODE *)newNode)->left = NULL;
    ((AVL_NODE *)newNode)->right = NULL;
    ((AVL_NODE *)newNode)->height = 1;
    *nodepp = newNode;

    avlRebalance ((AVL_NODE ***)ancestor, ancestorCount);

    return BS_OK;
}

/*******************************************************************************
 *
 * avlDelete - delete a node in an AVL tree
 *
 * At the time of the call, <root> poLONGs to the root node poLONGer and
 * <key> is the key of the node we want to delete. <compare> is the
 * user-provided comparison function.
 *
 * The deletion operation will of course fail if the desired node was not
 * already in the tree.
 *
 * Also note that because we keep the tree balanced, the root node poLONGer that
 * is poLONGed by the <root> argument can be modified in this function.
*
* On exit, the node is removed from the AVL tree but it is not free()'d.
*
* INTERNAL
* The deletion routine works just like in a non-balanced binary tree : we
* walk down the tree like searching the node we have to delete. When we find
* it, if it is not a leaf node, we have to replace it with a leaf node that
* has an adjacent key. Then the rebalancing operation will have to enforce the
* balancing rules by walking up the path from the leaf node that got deleted
* or moved.
*
* Because the balancing procedure needs to be able to walk back to the root
* node, we keep a list of poLONGers to the poLONGers we followed on our way down
* the tree.
*
* RETURNS: poLONGer to the node we deleted, or NULL if the tree does not
* contain any such node
*/

void * avlDelete(AVL_TREE *root, void *key, PF_AVL_CMP_FUNC cmp_func)
{
    AVL_NODE ** nodepp;             /* ptr to current node ptr */
    AVL_NODE *  nodep;              /* ptr to the current node */
    AVL_NODE ** ancestor[AVL_MAX_HEIGHT];   /* list of poLONGers to all our ancestor node poLONGers */
    int ancestorCount;              /* number of ancestors */
    AVL_NODE *  deletep;            /* ptr to the node we have to delete */

    nodepp = root;
    ancestorCount = 0;
    while (1)
    {
        int delta;      /* result of the comparison operation */

        nodep = *nodepp;
        if (nodep == NULL)
            return NULL;    /* node was not in the tree ! */

        ancestor[ancestorCount++] = nodepp;

        delta = cmp_func(key, nodep);
        if (0 == delta)
            break;      /* we found the node we have to delete */
        else if (delta < 0)
            nodepp = (AVL_NODE **)&(nodep->left);
        else
            nodepp = (AVL_NODE **)&(nodep->right);
    }

    deletep = nodep;

    if (nodep->left == NULL) {
        /*
         * There is no node on the left subtree of delNode.
         * Either there is one (and only one, because of the balancing rules)
         * on its right subtree, and it replaces delNode, or it has no child
         * nodes at all and it just gets deleted
         */

        *nodepp = nodep->right;

        /*
         * we know that nodep->right was already balanced so we don't have to
         * check it again
         */

        ancestorCount--;    
    } else {
        /*
         * We will find the node that is just before delNode in the ordering
         * of the tree and promote it to delNode's position in the tree.
         */

        AVL_NODE ** deletepp;       /* ptr to the ptr to the node
                                       we have to delete */
        int     deleteAncestorCount;    /* place where the replacing
                                           node will have to be
                                           inserted in the ancestor
                                           list */

        deleteAncestorCount = ancestorCount;
        deletepp = nodepp;
        deletep = nodep;

        /* search for node just before delNode in the tree ordering */

        nodepp = (AVL_NODE **)&(nodep->left);
        while (1)
        {
            nodep = *nodepp;
            if (nodep->right == NULL)
                break;
            ancestor[ancestorCount++] = nodepp;
            nodepp = (AVL_NODE **)&(nodep->right);
        }

        /*
         * this node gets replaced by its (unique, because of balancing rules)
         * left child, or deleted if it has no childs at all
         */

        *nodepp = nodep->left;

        /* now this node replaces delNode in the tree */

        nodep->left = deletep->left;
        nodep->right = deletep->right;
        nodep->height = deletep->height;
        *deletepp = nodep;

        /*
         * We have replaced delNode with nodep. Thus the poLONGer to the left
         * subtree of delNode was stored in delNode->left and it is now
         * stored in nodep->left. We have to adjust the ancestor list to
         * reflect this.
         */

        ancestor[deleteAncestorCount] = (AVL_NODE **)&(nodep->left);
    }

    avlRebalance (ancestor, ancestorCount);

    return deletep;
}

/*******************************************************************************
 *
 * avlDeleteUnsigned - delete a node in an AVL tree
 *
 * This is a specialized implementation of avlDelete for cases where the
 * node to be deleted is an AVL_UNSIGNED_NODE.
 *
 * RETURNS: poLONGer to the node we deleted, or NULL if the tree does not
 * contain any such node
 */

void * avlDeleteUnsigned(AVL_TREE *root, UINT key)
{
    AVL_UNSIGNED_NODE **    nodepp;     /* ptr to current node ptr */
    AVL_UNSIGNED_NODE *     nodep;      /* ptr to the current node */
    AVL_UNSIGNED_NODE **    ancestor[AVL_MAX_HEIGHT];
    /* list of poLONGers to all our ancestor node poLONGers */
    int             ancestorCount;  /* number of ancestors */
    AVL_UNSIGNED_NODE *     deletep;    /* ptr to the node we have to delete */

    nodepp = (AVL_UNSIGNED_NODE **)root;
    ancestorCount = 0;
    while (1) {
        nodep = *nodepp;
        if (nodep == NULL)
            return NULL;    /* node was not in the tree ! */

        ancestor[ancestorCount++] = nodepp;

        if (key == nodep->key)
            break;      /* we found the node we have to delete */
        else if (key < nodep->key)
            nodepp = (AVL_UNSIGNED_NODE **)&(nodep->avl.left);
        else
            nodepp = (AVL_UNSIGNED_NODE **)&(nodep->avl.right);
    }

    deletep = nodep;

    if (nodep->avl.left == NULL) {
        /*
         * There is no node on the left subtree of delNode.
         * Either there is one (and only one, because of the balancing rules)
         * on its right subtree, and it replaces delNode, or it has no child
         * nodes at all and it just gets deleted
         */

        *nodepp = nodep->avl.right;

        /*
         * we know that nodep->right was already balanced so we don't have to
         * check it again
         */

        ancestorCount--;    
    } else {
        /*
         * We will find the node that is just before delNode in the ordering
         * of the tree and promote it to delNode's position in the tree.
         */

        AVL_UNSIGNED_NODE **    deletepp;   /* ptr to the ptr to the node
                                               we have to delete */
        int deleteAncestorCount;    /* place where the replacing node will
                                       have to be inserted in the ancestor
                                       list */

        deleteAncestorCount = ancestorCount;
        deletepp = nodepp;
        deletep = nodep;

        /* search for node just before delNode in the tree ordering */

        nodepp = (AVL_UNSIGNED_NODE **)&(nodep->avl.left);
        while (1) {
            nodep = *nodepp;
            if (nodep->avl.right == NULL)
                break;
            ancestor[ancestorCount++] = nodepp;
            nodepp = (AVL_UNSIGNED_NODE **)&(nodep->avl.right);
        }

        /*
         * this node gets replaced by its (unique, because of balancing rules)
         * left child, or deleted if it has no childs at all
         */

        *nodepp = nodep->avl.left;

        /* now this node replaces delNode in the tree */

        nodep->avl.left = deletep->avl.left;
        nodep->avl.right = deletep->avl.right;
        nodep->avl.height = deletep->avl.height;
        *deletepp = nodep;

        /*
         * We have replaced delNode with nodep. Thus the poLONGer to the left
         * subtree of delNode was stored in delNode->left and it is now
         * stored in nodep->left. We have to adjust the ancestor list to
         * reflect this.
         */

        ancestor[deleteAncestorCount] = (AVL_UNSIGNED_NODE **)&(nodep->avl.left);
    }

    avlRebalance ((AVL_NODE ***)ancestor, ancestorCount);

    return deletep;
}

/*******************************************************************************
 *
 * avlSuccessorGet - find node with key successor to input key on an AVL tree
 *
 * At the time of the call, <root> is the root node poLONGer. <key> is the value
 * we want to search, and <compare> is the user-provided comparison function.
 *
 * Note that we cannot have several nodes with the equal keys in the tree, so
 * there is no ambiguity about which node will be found.
 *
 * Also note that the search procedure does not depend on the tree balancing
 * rules, but because the tree is balanced, we know that the search procedure
 * will always be efficient.
 *
 * RETURNS: poLONGer to the node whose key is the immediate successor of <key>,
 * or NULL if there is no such node in the tree
*/
/* 获取比key大的最小的节点 */
void * avlSuccessorGet(AVL_TREE root, IN void *key, PF_AVL_CMP_FUNC cmp_func)
{
    AVL_NODE *  nodep;  /* poLONGer to the current node */
    AVL_NODE *  superiorp;  /* poLONGer to the current superior*/

    nodep = root;
    superiorp = NULL;
    while (1) {
        int delta;  /* result of the comparison operation */

        if (nodep == NULL)
            return superiorp;

        delta = cmp_func(key, nodep);
        if (delta < 0) {
            superiorp = nodep; /* update superiorp */
            nodep = nodep->left;
        } else {
            nodep = nodep->right;
        }
    }
}

/*******************************************************************************
 *
 * avlPredecessorGet - find node with key predecessor to input key on an AVL tree
 *
 * At the time of the call, <root> is the root node poLONGer. <key> is the value
 * we want to search, and <compare> is the user-provided comparison function.
 *
 * Note that we cannot have several nodes with the equal keys in the tree, so
 * there is no ambiguity about which node will be found.
 *
 * Also note that the search procedure does not depend on the tree balancing
 * rules, but because the tree is balanced, we know that the search procedure
 * will always be efficient.
 *
 * RETURNS: poLONGer to the node whose key is the immediate predecessor of <key>,
 * or NULL if there is no such node in the tree
 */
/* 获取比key小的最大的节点 */
VOID * avlPredecessorGet (AVL_TREE root, IN void *key, PF_AVL_CMP_FUNC cmp_func)
{
    AVL_NODE *  nodep;  /* poLONGer to the current node */
    AVL_NODE *  inferiorp;  /* poLONGer to the current inferior*/

    nodep = root;
    inferiorp = NULL;

    while (1) {
        int delta;  /* result of the comparison operation */

        if (nodep == NULL)
            return inferiorp;

        delta = cmp_func(key, nodep);
        if (delta > 0) {
            inferiorp = nodep; /* update inferiorp */
            nodep = nodep->right;
        } else {
            nodep = nodep->left;
        }
    }
}

/*******************************************************************************
 *
 * avlMinimumGet - find node with minimum key
 *
 * At the time of the call, <root> is the root node poLONGer. <key> is the value
 * we want to search, and <compare> is the user-provided comparison function.
 *
 * RETURNS: poLONGer to the node with minimum key; NULL if the tree is empty
 */

void * avlMinimumGet(AVL_TREE root)
{
    if  (NULL == root)
        return NULL;

    while (root->left != NULL) {
        root = root->left;
    }

    return root;
}

/*******************************************************************************
 *
 * avlMaximumGet - find node with maximum key
 *
 * At the time of the call, <root> is the root node poLONGer. <key> is the value
 * we want to search, and <compare> is the user-provided comparison function.
 *
 * RETURNS: poLONGer to the node with maximum key; NULL if the tree is empty
 */

void * avlMaximumGet (AVL_TREE root)
{
    if  (NULL == root)
        return NULL;

    while (root->right != NULL) {
        root = root->right;
    }

    return root;
}

/*******************************************************************************
 *
 * avlInsertInform - insert a node in an AVL tree and report key holder
 *
 * At the time of the call, <pRoot> poLONGs to the root node poLONGer. This root
 * node poLONGer is possibly NULL if the tree is empty. <pNewNode> poLONGs to the
 * node we want to insert. His left, right and height fields need not be filled,
 * but the user will probably have added his own data fields after those. <key>
 * is newNode's key, that will be passed to the comparison function. This is
 * redundant because it could really be derived from newNode, but the way to do
 * this depends on the precise type of newNode so we cannot do this in a generic
 * routine. <compare> is the user-provided comparison function.
 *
 * Note that we cannot have several nodes with the equal keys in the tree, so
 * the insertion operation will fail if we try to insert a node that has a
 * duplicate key. However, if the <replace> boolean is set to true then in
 * case of conflict we will remove the old node, we will put in its position the
 * new one, and we will return the old node poLONGer in the postion poLONGed by
 * <ppReplacedNode>.
 *
 * Also note that because we keep the tree balanced, the root node poLONGer that
 * is poLONGed by the <pRoot> argument can be modified in this function.
 *
 * INTERNAL
 * The insertion routine works just like in a non-balanced binary tree : we
 * walk down the tree like if we were searching a node, and when we reach a leaf
* node we insert newNode at this position.
*
* Because the balancing procedure needs to be able to walk back to the root
* node, we keep a list of poLONGers to the poLONGers we followed on our way down
* the tree.
*
* RETURNS: BS_OK, or BS_ERR if the tree already contained a node with the same key
* and replacement was not allowed. In both cases it will place a poLONGer to
* the key holder in the position poLONGed by <ppKeyHolder>.
*/

int avlInsertInform (AVL_TREE *pRoot, void *pNewNode, void *key, void **ppKeyHolder, PF_AVL_CMP_FUNC cmp_func)
{
    AVL_NODE ** nodepp;             /* ptr to current node ptr */
    AVL_NODE ** ancestor[AVL_MAX_HEIGHT];   /* list of poLONGers to all
                                               our ancestor node ptrs */
    int     ancestorCount;          /* number of ancestors */

    if  (NULL == ppKeyHolder) {
        RETURN(BS_ERR);
    }

    nodepp = pRoot;
    ancestorCount = 0;

    while (1) {
        AVL_NODE *  nodep;  /* poLONGer to the current node */
        int     delta;  /* result of the comparison operation */

        nodep = *nodepp;
        if (nodep == NULL)
            break;  /* we can insert a leaf node here ! */

        ancestor[ancestorCount++] = nodepp;

        delta = cmp_func(key, nodep);
        if  (0 == delta) {
            /* we inform the caller of the key holder node and return BS_ERR */
            *ppKeyHolder = nodep;
            RETURN(BS_ERR);
        } else if (delta < 0) {
            nodepp = (AVL_NODE **)&(nodep->left);
        } else {
            nodepp = (AVL_NODE **)&(nodep->right);
        }
    }

    ((AVL_NODE *)pNewNode)->left = NULL;
    ((AVL_NODE *)pNewNode)->right = NULL;
    ((AVL_NODE *)pNewNode)->height = 1;
    *nodepp = pNewNode;

    *ppKeyHolder = pNewNode;

    avlRebalance (ancestor, ancestorCount);

    return BS_OK;
}

/*******************************************************************************
 *
 * avlRemoveInsert - forcefully insert a node in an AVL tree
 *
 * At the time of the call, <pRoot> poLONGs to the root node poLONGer. This root
 * node poLONGer is possibly NULL if the tree is empty. <pNewNode> poLONGs to the
 * node we want to insert. His left, right and height fields need not be filled,
 * but the user will probably have added his own data fields after those. <key>
 * is newNode's key, that will be passed to the comparison function. This is
 * redundant because it could really be derived from newNode, but the way to do
 * this depends on the precise type of newNode so we cannot do this in a generic
 * routine. <compare> is the user-provided comparison function.
 *
 * Note that we cannot have several nodes with the equal keys in the tree, so
 * the insertion operation will fail if we try to insert a node that has a
 * duplicate key. However, in case of conflict we will remove the old node, we 
 * will put in its position the new one, and we will return the old node poLONGer
 *
 * Also note that because we keep the tree balanced, the root node poLONGer that
 * is poLONGed by the <pRoot> argument can be modified in this function.
 *
 * INTERNAL
 * The insertion routine works just like in a non-balanced binary tree : we
 * walk down the tree like if we were searching a node, and when we reach a leaf
 * node we insert newNode at this position.
 *
 * Because the balancing procedure needs to be able to walk back to the root
 * node, we keep a list of poLONGers to the poLONGers we followed on our way down
 * the tree.
 *
 * RETURNS: NULL if insertion was carried out without replacement, or if 
 * replacement occured the poLONGer to the replaced node
 *
 */

void * avlRemoveInsert (AVL_TREE * pRoot, void *pNewNode, void *key, PF_AVL_CMP_FUNC cmp_func)
{
    AVL_NODE ** nodepp;             /* ptr to current node ptr */
    AVL_NODE ** ancestor[AVL_MAX_HEIGHT];   /* list of poLONGers to all
                                               our ancestor node ptrs */
    int     ancestorCount;          /* number of ancestors */

    nodepp = pRoot;
    ancestorCount = 0;

    while (1) {
        AVL_NODE *  nodep;  /* poLONGer to the current node */
        int     delta;  /* result of the comparison operation */

        nodep = *nodepp;
        if (nodep == NULL)
            break;  /* we can insert a leaf node here ! */

        ancestor[ancestorCount++] = nodepp;

        delta = cmp_func(key, nodep);
        if  (0 == delta) {
            /* we copy the tree data from the old node to the new node */
            ((AVL_NODE *)pNewNode)->left = nodep->left;
            ((AVL_NODE *)pNewNode)->right = nodep->right;
            ((AVL_NODE *)pNewNode)->height = nodep->height;

            /* and we make the new node child of the old node's parent */
            *nodepp = pNewNode;

            /* before we return it we sterilize the old node */
            nodep->left = NULL;
            nodep->right = NULL;
            nodep->height = 1;

            return nodep;
        }
        else if (delta < 0)
            nodepp = (AVL_NODE **)&(nodep->left);
        else
            nodepp = (AVL_NODE **)&(nodep->right);
    }

    ((AVL_NODE *)pNewNode)->left = NULL;
    ((AVL_NODE *)pNewNode)->right = NULL;
    ((AVL_NODE *)pNewNode)->height = 1;
    *nodepp = pNewNode;

    avlRebalance (ancestor, ancestorCount);

    return NULL;
}


/*******************************************************************************
 *
 * avlTreeWalk- walk the whole tree and execute a function on each node
 *
 * At the time of the call, <pRoot> poLONGs to the root node poLONGer.
 *
 */

BS_WALK_RET_E avlTreeWalk(AVL_TREE * pRoot, PF_AVL_WALK_FUNC walk_func, void *ud)
{
    BS_WALK_RET_E ret;

    if  ((NULL == pRoot) || (NULL == *pRoot)) {
        return BS_WALK_CONTINUE;
    }

    if  (!(NULL == (*pRoot)->left)) {
        ret = avlTreeWalk((AVL_TREE *)(&((*pRoot)->left)), walk_func, ud);
        if (ret != BS_WALK_CONTINUE) {
            return ret;
        }
    }

    ret = walk_func(*pRoot, ud);
    if (ret != BS_WALK_CONTINUE) {
        return ret;
    }

    if  (NULL == (*pRoot)->right) {
        return BS_WALK_CONTINUE;
    }

    return avlTreeWalk((AVL_TREE *)(&((*pRoot)->right)), walk_func, ud);
}

/*******************************************************************************
 *
 * avlTreePrLONG- prLONG the whole tree
 *
 * At the time of the call, <pRoot> poLONGs to the root node poLONGer.
 *
 * RETURNS: BS_OK always
 *
 */

BS_STATUS avlTreePrint(AVL_TREE * pRoot, PF_AVL_PRINT_FUNC print_func)
{
    if  ((NULL == pRoot) || (NULL == *pRoot))
    {
        return BS_OK;
    }

    print_func(*pRoot);

    if  (!(NULL == (*pRoot)->left))
    {
        avlTreePrint((AVL_TREE *)(&((*pRoot)->left)), print_func);
    }

    if  (!(NULL == (*pRoot)->right))
    {
        avlTreePrint((AVL_TREE *)(&((*pRoot)->right)), print_func);
    }

    return BS_OK;
}

/*******************************************************************************
*
* avlTreeErase - erase the whole tree
*
* At the time of the call, <pRoot> poLONGs to the root node poLONGer.
* Unlike the avlDelete routine here we do perform memory management
* ASSUMING that all nodes carry shallow data only. Otherwise we should
* use avlTreeWalk with the appropriate walkExec memory freeing function.
* Since we do not plan to reuse the tree LONGermediate rebalancing is not needed.
*
* RETURNS: BS_OK always
*
*/

BS_STATUS avlTreeErase(AVL_TREE * pRoot, IN PF_AVL_FREE_FUNC pfFree, IN void *pUserHandle)
{
    if  ((NULL == pRoot) || (NULL == *pRoot)) {
        return BS_OK;
    }

    if  (!(NULL == (*pRoot)->left)) {
        avlTreeErase((AVL_TREE *)(&((*pRoot)->left)), pfFree, pUserHandle);
    }

    if  (!(NULL == (*pRoot)->right)) {
        avlTreeErase((AVL_TREE *)(&((*pRoot)->right)), pfFree, pUserHandle);
    }

    pfFree(*pRoot, pUserHandle);

    *pRoot = NULL;

    return BS_OK;
}

/*******************************************************************************
*
* avlTreePrintErase - erase the whole tree assuming that all nodes were
* created using malloc.
*
* At the time of the call, <pRoot> poLONGs to the root node poLONGer.
* Unlike the avlDelete routine here we do perform memory management.
* Since we do not plan to reuse the tree LONGermediate rebalancing is not needed.
*
* RETURNS: BS_OK always and assigns NULL to *pRoot.
*
*/

BS_STATUS avlTreePrintErase(AVL_TREE * pRoot, PF_AVL_PRINT_FUNC print_func)
{
    if  ((NULL == pRoot) || (NULL == *pRoot))
    {
        return BS_OK;
    }

    print_func(*pRoot);

    if  (!(NULL == (*pRoot)->left))
    {
        avlTreePrintErase((AVL_TREE *)(&((*pRoot)->left)), print_func);
    }

    if  (!(NULL == (*pRoot)->right))
    {
        avlTreePrintErase((AVL_TREE *)(&((*pRoot)->right)), print_func);
    }

    MEM_Free(*pRoot);
    *pRoot = NULL;

    return BS_OK;
}

