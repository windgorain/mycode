/*********************************************************
*   Copyright (C) LiXingang
*   Description: 用于查找多个表,将这些表的结果进行bit与操作得到结果
*                比如用于ACL的匹配
*
*********************************************************/
#include "bs.h"
#include "utl/array_bit.h"
#include "utl/bitmatch_utl.h"

typedef struct tagBITMATCH_S {
    UINT tab_number;
    UINT max_rule_num;
    UCHAR data[0];
}BITMATCH_S;

static inline UCHAR * bitmatch_get_bits(BITMATCH_S *ctrl, int tbl_index, int pos)
{
    UCHAR *bits;

    BS_DBGASSERT(tbl_index < ctrl->tab_number);

    /* 先定位tbl_index所对应的表的起始地址 */
    bits = ctrl->data + ((ctrl->max_rule_num / 8) * tbl_index * 256);

    /* 再定位pos所对应的位图表 */
    bits += ((ctrl->max_rule_num / 8) * tbl_index * pos);

    return bits;
}

int BITMATCH_Init(INOUT BITMATCH_S *ctrl)
{
    int data_size = ctrl->tab_number * ctrl->max_rule_num / 8;

    BS_DBGASSERT(ctrl->max_rule_num % 32 == 0);

    if (data_size <= 0) {
        RETURN(BS_ERR);
    }
    memset(ctrl->data, 0, data_size);

    return 0;
}

BITMATCH_S * BITMATCH_Create(int tab_number, int max_rule_num)
{
    int data_size = tab_number * max_rule_num / 8;

    BS_DBGASSERT(max_rule_num % 32 == 0);

    BITMATCH_S *ctrl = MEM_ZMalloc(sizeof(BITMATCH_S) + data_size);
    if (! ctrl) {
        return NULL;
    }

    ctrl->tab_number = tab_number;
    ctrl->max_rule_num = max_rule_num;

    return ctrl;
}

void BITMATCH_Destroy(BITMATCH_S *ctrl)
{
    if (ctrl) {
        MEM_Free(ctrl);
    }
}

void BITMATCH_AddRule(BITMATCH_S *ctrl, UINT tbl_index, UCHAR range_min, UCHAR range_max, UINT rule_id)
{
    int i;
    void *bits;

    BS_DBGASSERT(tbl_index < ctrl->tab_number);
    BS_DBGASSERT(rule_id < ctrl->max_rule_num);
    BS_DBGASSERT(range_max >= range_min);

    for (i=range_min; i<=range_max; i++) {
        bits = bitmatch_get_bits(ctrl, tbl_index, i);
        ArrayBit_Set(bits, rule_id);
    }
}

void BITMATCH_DelRule(BITMATCH_S *ctrl, UINT tbl_index, UCHAR range_min, UCHAR range_max, UINT rule_id)
{
    int i;
    void *bits;

    BS_DBGASSERT(tbl_index < ctrl->tab_number);
    BS_DBGASSERT(rule_id < ctrl->max_rule_num);
    BS_DBGASSERT(range_max >= range_min);

    for (i=range_min; i<=range_max; i++) {
        bits = bitmatch_get_bits(ctrl, tbl_index, i);
        ArrayBit_Clr(bits, rule_id);
    }
}

/* data的长度必须是ctrl->tab_number字节数 */
int BITMATCH_Match(BITMATCH_S *ctrl, void *data, OUT void *matched_bits)
{
    int i;
    UCHAR *d = data;
    void *bits;
    int bits_size = ctrl->max_rule_num / 8;

    bits = bitmatch_get_bits(ctrl, 0, d[0]);

    memcpy(matched_bits, bits, bits_size);

    for (i=1; i<ctrl->tab_number; i++) {
        bits = bitmatch_get_bits(ctrl, i, d[i]);
        ArrayBit_And(matched_bits, bits, ctrl->max_rule_num / 32, matched_bits);
    }

    return 0;
}

