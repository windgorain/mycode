/*================================================================
*   Created by LiXingang
*   Description: OB: observer 叮当
*
================================================================*/
#include "bs.h"

#include "utl/ob_utl.h"

static void ob_AddWithPri(OB_LIST_S *head, OB_S *ob)
{
    OB_S *tmp_ob;

    DLL_SCAN(head, tmp_ob) {
        if (ob->pri <= tmp_ob->pri) {
            DLL_INSERT_BEFORE(head, ob, tmp_ob);
            return;
        }
    }

    DLL_ADD(head, ob);

    return;
}

void OB_Add(OB_LIST_S *head, OB_S *ob)
{
    return ob_AddWithPri(head, ob);
}

void OB_Del(OB_LIST_S *head, OB_S *ob)
{
    DLL_DEL(head, ob);
}
