/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
*************************************************/
#ifndef _BITMATCH_UTL_H
#define _BITMATCH_UTL_H
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct tagBITMATCH_S BITMATCH_S;

int BITMATCH_Init(INOUT BITMATCH_S *ctrl);
BITMATCH_S * BITMATCH_Create(int tab_number, int max_rule_num);
void BITMATCH_Destroy(BITMATCH_S *ctrl);
void BITMATCH_AddRule(BITMATCH_S *ctrl, UINT tbl_index, UCHAR range_min, UCHAR range_max, UINT rule_id);
void BITMATCH_DelRule(BITMATCH_S *ctrl, UINT tbl_index, UCHAR range_min, UCHAR range_max, UINT rule_id);
int BITMATCH_Match(BITMATCH_S *ctrl, void *data, OUT void *matched_bits);

#ifdef __cplusplus
}
#endif
#endif //BITMATCH_UTL_H_
