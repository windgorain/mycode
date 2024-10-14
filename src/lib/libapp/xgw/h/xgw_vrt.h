/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0
* Description:
******************************************************************************/
#ifndef _XGW_VRT_H
#define _XGW_VRT_H

#include "utl/lpm_utl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define XGW_VNI_MAX (256*256*256)

typedef int (*PF_XGW_VRT_Walk)(U32 vni, U32 ip, int depth, U64 nexthop, void *ud);

int XGW_VRT_Init(void);
int XGW_VRT_AddVni(U32 vni);
void XGW_VRT_DelVni(U32 vni);
int XGW_VRT_AddRoute(U32 vni, U32 dip, U8 depth, U64 nexthop);
int XGW_VRT_DelRoute(U32 vni, U32 dip, U8 depth);
U64 XGW_VRT_Match(U32 vni, U32 dip);
int XGW_VRT_Walk(PF_XGW_VRT_Walk walk_func, void *ud);
int XGW_VRT_WalkVni(U32 vni, PF_LPM_WALK_CB walk_func, void *ud);

#ifdef __cplusplus
}
#endif
#endif 
