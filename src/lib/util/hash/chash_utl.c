/*================================================================
*   Created by LiXingang
*   Description: 一致性hash
*
================================================================*/
#include "bs.h"
#include "utl/skip_list.h"
#include "utl/chash_utl.h"
#include "utl/jhash_utl.h"

typedef struct {
    SKL_HDL hSkl;
}CHASH_S;

CHASH_HDL CHASH_Create()
{
    CHASH_S *ctrl = MEM_ZMalloc(sizeof(CHASH_S));
    if (! ctrl) {
        return NULL;
    }

    ctrl->hSkl = SKL_Create(32);
    if (! ctrl->hSkl) {
        MEM_Free(ctrl);
        return NULL;
    }

    return ctrl;
}

int CHASH_AddNode(CHASH_HDL hCtrl, void *key, int key_len, int vcount, void *node)
{
    CHASH_S *ctrl = hCtrl;
    int i;
    UINT hash;

    for (i=0; i<vcount; i++) {
        hash = JHASH_GeneralBuffer(key, key_len, i);
        SKL_Insert(ctrl->hSkl, hash, node);
    }

    return 0;
}

int CHASH_DelNode(CHASH_HDL hCtrl, void *key, int key_len, int vcount, void *node)
{
    CHASH_S *ctrl = hCtrl;
    int i;
    UINT hash;
    void *data;

    for (i=0; i<vcount; i++) {
        hash = JHASH_GeneralBuffer(key, key_len, i);
        data = SKL_Search(ctrl->hSkl, hash);
        if (data == node) {
            SKL_Delete(ctrl->hSkl, hash);
        }
    }

    return 0;
}

void * CHASH_GetNode(CHASH_HDL hCtrl, UINT hash)
{
    CHASH_S *ctrl = hCtrl;
    void *node;

    node = SKL_GetInRight(ctrl->hSkl, hash);
    if (! node) {
        node = SKL_GetFirst(ctrl->hSkl);
    }

    return node;
}

