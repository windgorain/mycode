/*********************************************************
*   Copyright (C) LiXingang
*   Description: 用于查找多个表,将这些表的结果进行bit与操作得到结果
*                比如用于ACL的匹配
*
*********************************************************/
#include "bs.h"
#include "utl/array_bit.h"
#include "utl/bitmatch_utl.h"

int BITMATCH_Init(INOUT BITMATCH_S *ctrl)
{
    int size = ctrl->tab_number * ctrl->max_rule_num / 8;

    BS_DBGASSERT(ctrl->max_rule_num % 32 == 0);

    if (size <= 0) {
        RETURN(BS_ERR);
    }

    memset(ctrl->rule_bits, 0, size);

    return 0;
}

BITMATCH_S * BITMATCH_Create(int tab_number, int max_rule_num)
{
    int size = tab_number * max_rule_num / 8;

    BS_DBGASSERT(max_rule_num % 32 == 0);

    BITMATCH_S *ctrl = MEM_ZMalloc(sizeof(BITMATCH_S));
    if (! ctrl) {
        return NULL;
    }

    ctrl->rule_bits = MEM_ZMalloc(size);
    if (! ctrl->rule_bits) {
        MEM_Free(ctrl);
        return NULL;
    }

    ctrl->tab_number = tab_number;
    ctrl->max_rule_num = max_rule_num;

    return ctrl;
}

void BITMATCH_Destroy(BITMATCH_S *ctrl)
{
    if (ctrl) {
        return;
    }

    if (ctrl->rule_bits) {
        MEM_Free(ctrl->rule_bits);
    }

    MEM_Free(ctrl);
}

void BITMATCH_AddRule(BITMATCH_S *ctrl, UINT tbl_index, UCHAR range_min, UCHAR range_max, UINT rule_id)
{
    int i;
    void *bits;

    BS_DBGASSERT(tbl_index < ctrl->tab_number);
    BS_DBGASSERT(rule_id < ctrl->max_rule_num);
    BS_DBGASSERT(range_max >= range_min);

    for (i=range_min; i<=range_max; i++) {
        bits = _bitmatch_get_bits(ctrl->max_rule_num, ctrl->rule_bits, tbl_index, i);
        ArrayBit_Set(bits, rule_id);
    }
}

void BITMATCH_DelRule(BITMATCH_S *ctrl, UINT rule_id)
{
    int i, j;
    void *bits;

    BS_DBGASSERT(rule_id < ctrl->max_rule_num);

    for (i=0; i<ctrl->tab_number; i++) {
        for (j=0; j<=255; j++) {
            bits = _bitmatch_get_bits(ctrl->max_rule_num, ctrl->rule_bits, i, j);
            ArrayBit_Clr(bits, rule_id);
        }
    }
}

void BITMATCH_GetRuleBits(BITMATCH_S *ctrl, OUT void *rule_bits)
{
    int i, j;
    void *bits;
    int size = ctrl->max_rule_num / 8;

    memset(rule_bits, 0, size);

    for (i=0; i<ctrl->tab_number; i++) {
        for (j=0; j<=255; j++) {
            bits = _bitmatch_get_bits(ctrl->max_rule_num, ctrl->rule_bits, i, j);
            ArrayBit_Or(rule_bits, bits, ctrl->max_rule_num / 32, rule_bits);
        }
    }
}

int BITMATCH_GetRule(BITMATCH_S *ctrl, U32 rule_id, OUT void *min, OUT void *max)
{
    int tbl_index, i;
    void *bits;
    U8 *d_min, *d_max;
    int ret = -1;


    d_min = min;
    d_max = max;

    memset(d_min, 0, ctrl->tab_number);
    memset(d_max, 0, ctrl->tab_number);

    for (tbl_index = 0; tbl_index < ctrl->tab_number; tbl_index++) {
        int found = 0;

        
        for (i=0; i<=255; i++) {
            bits = _bitmatch_get_bits(ctrl->max_rule_num, ctrl->rule_bits, tbl_index, i);
            if (ArrayBit_Test(bits, rule_id)) {
                d_min[tbl_index] = i;
                found = 1;
                ret = 0;
                break;
            }
        }

        if (! found) {
            continue;
        }

        
        for (i=255; i>=d_min[tbl_index]; i--) {
            bits = _bitmatch_get_bits(ctrl->max_rule_num, ctrl->rule_bits, tbl_index, i);
            if (ArrayBit_Test(bits, rule_id)) {
                d_max[tbl_index] = i;
                break;
            }
        }
    }

    return ret;
}


