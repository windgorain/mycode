/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/net.h"
#include "utl/ipmask_bitmap.h"


void IPMASK_BITMAP_Init(IPMASK_BITMAP_S *ctrl, UCHAR depth)
{
    UINT count;

    BS_DBGASSERT(depth >= 3);

    count = PREFIX_2_CAPACITY(depth);

    ctrl->depth = depth;
    memset(ctrl->bits, 0, count>>3);
}

static int ipmaskbitmap_Set(IPMASK_BITMAP_S *ctrl, UINT ip, UINT mask, int on)
{
    int i;
    UINT offset = 0;
    UINT start;
    UINT index;
    UINT ctrl_mask;
    UINT ctrl_count;

    ctrl_mask = PREFIX_2_MASK(ctrl->depth);
    ctrl_count = PREFIX_2_COUNT(ctrl->depth);

    if ((mask & (~ctrl_mask)) != 0) {
        return BS_BAD_PARA;
    }

    offset = (mask ^ ctrl_mask) >> (32 - ctrl->depth);
    start = (ip & mask) >> (32 - ctrl->depth);

    for (i = 0; i <= offset; i++) {
        index = start + i;
        if (index < ctrl_count) {
            if (on) {
                ctrl->bits[index>>3] |= (1UL<<(index & 7));
            } else {
                ctrl->bits[index>>3] &= (~(1UL<<(index & 7)));
            }
        }
    }

    return 0;
}

int IPMASK_BITMAP_Add(IPMASK_BITMAP_S *ctrl, UINT ip, UINT mask)
{
    return ipmaskbitmap_Set(ctrl, ip, mask, 1);
}

int IPMASK_BITMAP_Del(IPMASK_BITMAP_S *ctrl, UINT ip, UINT mask)
{
    return ipmaskbitmap_Set(ctrl, ip, mask, 0);
}

int IPMASK_BITMAP_IsSet(IPMASK_BITMAP_S *ctrl, UINT ip)
{
    uint32_t index;

    index = ip >> (32 - ctrl->depth);
    return (ctrl->bits[index>>3] & (1UL<<(index & 7))) ? 1 : 0;
}

void IPMASK_BITMAP_Show(IPMASK_BITMAP_S *ctrl)
{
    int i;
    UINT start = 0;
    UINT end = 0;
    UINT prev = 0;
    UINT ctrl_mask;
    UINT ctrl_count;

    ctrl_mask = PREFIX_2_MASK(ctrl->depth);
    ctrl_count = PREFIX_2_COUNT(ctrl->depth);
 
    for (i = 0; i < ctrl_count; i++) {
        if (ctrl->bits[i>>3] & (1UL << (i & 7))) {
            if (prev == 0) {
                start = i;
            }
            end = i;
            if (i == (ctrl_count - 1)) {
                printf("%u.%u.%u.%u - %u.%u.%u.%u\n",
                       PRINT_HIP(start << (32 - ctrl->depth)),
                       PRINT_HIP((end << (32 - ctrl->depth)) + (~ctrl_mask)));
            }
            prev = 1;
        } else {
            if (prev != 0) {
                printf("%u.%u.%u.%u - %u.%u.%u.%u\n",
                       PRINT_HIP(start << (32 - ctrl->depth)),
                       PRINT_HIP((end << (32 - ctrl->depth)) + (~ctrl_mask)));
            }
            prev = 0;
        }
    }

    return ;
}

