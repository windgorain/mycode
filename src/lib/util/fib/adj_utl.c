/*================================================================
*   Created：2018.12.08 LiXingang All rights reserved.
*   Description：邻接表
*
================================================================*/
#include "bs.h"

#include "utl/adj_utl.h"


int ADJ_Init(IN ADJ_S *adj, IN UINT size, ADJ_NODE_S *array)
{
    adj->array = array;
    adj->size = size;

    return 0;
}

int ADJ_Add(IN ADJ_S *adj, IN UINT nexthop)
{
    int i;

    for (i=0; i<adj->size; i++) {
        if (adj->array[i].nexthop == 0) {
            adj->array[i].nexthop = nexthop;
            return i;
        }
    }

    RETURN(BS_NO_RESOURCE);
}

int ADJ_Find(IN ADJ_S *adj, IN UINT nexthop)
{
    int i;
    for (i=0; i<adj->size; i++) {
        if (adj->array[i].nexthop == nexthop) {
            return i;
        }
    }

    return -1;
}

void ADJ_DelByIndex(IN ADJ_S *adj, IN int index)
{
    if (index >= adj->size) {
        return;
    }

    adj->array[index].nexthop = 0;
}

void ADJ_DelByNexthop(IN ADJ_S *adj, IN UINT nexthop)
{
    int i;
    for (i=0; i<adj->size; i++) {
        if (adj->array[i].nexthop == nexthop) {
            adj->array[i].nexthop = 0;
        }
    }
}

