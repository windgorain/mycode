/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
*************************************************/
#ifndef _BITMATCH_UTL_H
#define _BITMATCH_UTL_H

#include "utl/array_bit.h"
#include "utl/bit_opt.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tagBITMATCH_S {
    UINT tab_number;
    UINT max_rule_num;
    UCHAR *rule_bits;
}BITMATCH_S;

int BITMATCH_Init(INOUT BITMATCH_S *ctrl);
BITMATCH_S * BITMATCH_Create(int tab_number, int max_rule_num);
void BITMATCH_Destroy(BITMATCH_S *ctrl);
void BITMATCH_AddRule(BITMATCH_S *ctrl, UINT tbl_index, UCHAR range_min, UCHAR range_max, UINT rule_id);
void BITMATCH_DelRule(BITMATCH_S *ctrl, UINT rule_id);
void BITMATCH_GetRuleBits(BITMATCH_S *ctrl, OUT void *rule_bits);
int BITMATCH_GetRule(BITMATCH_S *ctrl, U32 rule_id, OUT void *min, OUT void *max);

static inline UCHAR * _bitmatch_get_bits(U32 max_rule, U8 *rules_bits, int tbl_index, int pos)
{
    UCHAR *bits;

    
    bits = rules_bits + ((max_rule / 8) * tbl_index * 256);

    
    bits += ((max_rule/ 8) * pos);

    return bits;
}


static inline U32 _bitmatch_get_u32(U32 max_rule, void *rules_bits, int tbl_index, int pos, int offset)
{
    U32 * d = (void*)_bitmatch_get_bits(max_rule, rules_bits, tbl_index, pos);
    return d[offset];
}


static inline INT64 BITMATCH_MatchFirst(U32 tab_num, U32 max_rule, void *rule_bits, void *data)
{
    int i;
    int offset;
    UCHAR *d = data;
    U32 tmp1, tmp2;

    for (offset=0; offset<max_rule/32; offset++) {
        tmp1 = _bitmatch_get_u32(max_rule, rule_bits, 0, d[0], offset);
        for (i=1; i<tab_num; i++) {
            tmp2 = _bitmatch_get_u32(max_rule, rule_bits, i, d[i], offset);
            tmp1 = tmp1 & tmp2;
        }
        if (tmp1) {
            return (offset * 32) + BIT_GetLowIndex(tmp1);
        }
    }

    return -1;
}

static inline void BITMATCH_Match(U32 tab_num, U32 max_rule, void *rule_bits, void *data, OUT void *matched_bits)
{
    int i;
    UCHAR *d = data;
    void *bits;
    int size = max_rule / 8;

    bits = _bitmatch_get_bits(max_rule, rule_bits, 0, d[0]);

    memset(matched_bits, 0xff, size);

    for (i=0; i<tab_num; i++) {
        bits = _bitmatch_get_bits(max_rule, rule_bits, i, d[i]);
        ArrayBit_And(matched_bits, bits, max_rule / 32, matched_bits);
    }
}


static inline void BITMATCH_DoMatch(BITMATCH_S *ctrl, void *data, OUT void *matched_bits)
{
    BITMATCH_Match(ctrl->tab_number, ctrl->max_rule_num, ctrl->rule_bits, data, matched_bits);
}

#ifdef __cplusplus
}
#endif
#endif 
