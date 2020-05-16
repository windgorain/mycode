/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/num_utl.h"
#include "utl/atomic_utl.h"
#include "utl/ring_utl.h"

int RING_Init(RING_S *r, UINT capacity, int is_single_prod, int is_single_cons)
{
    if (! NUM_IS2N(capacity)) {
        RETURN(BS_BAD_PARA);
    }

    memset(r, 0, sizeof(RING_S));
    r->capacity = capacity;
    r->size = capacity;
    r->mask = r->size - 1;
    if (is_single_prod) {
        r->prod.single = 1;
    }
    if (is_single_cons) {
        r->cons.single = 1;
    }

    return 0;
}

void RING_Reset(RING_S *r)
{
    RING_Init(r, r->capacity, r->prod.single, r->cons.single);
}

RING_S * RING_Create(UINT capacity, int is_single_prod, int is_single_cons)
{
    RING_S *r;
    UINT count = capacity;

    if (! NUM_IS2N(count)) {
        count = NUM_To2N(count);
    }

    UINT len = sizeof(RING_S) + count * sizeof(void*);

    r = MEM_Malloc(len);
    if (r) {
        RING_Init(r, count, is_single_prod, is_single_cons);
    }

    return r;
}

void RING_Destroy(RING_S *r)
{
    if (r) {
        MEM_Free(r);
    }
}

