/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0
* Description:
******************************************************************************/
#ifndef _XGW_VHT_H
#define _XGW_VHT_H

#ifdef __cplusplus
extern "C" {
#endif

int XGW_VHT_Init(void);
int XGW_VHT_Add(U32 vid, U32 oip, U32 hip);
int XGW_VHT_Del(U32 vid, U32 oip);
U32 XGW_VHT_GetHip(U32 vid, U32 oip);
void XGW_VHT_DelAll(void);
void XGW_VHT_DelVid(U32 vid);
typedef int (*PF_XGW_VHT_WALK)(U32 vid, U32 oip, U32 hip, void *ud);
int XGW_VHT_Walk(PF_XGW_VHT_WALK walk_func, void *ud);

#ifdef __cplusplus
}
#endif
#endif 
