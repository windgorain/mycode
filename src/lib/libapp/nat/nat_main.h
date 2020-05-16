/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2013-1-9
* Description: 
* History:     
******************************************************************************/

#ifndef __NAT_MAIN_H_
#define __NAT_MAIN_H_

#include "utl/mbuf_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define NAT_MAIN_DBG_PACKET 0x1

BS_STATUS NAT_Main_Start();
VOID NAT_Main_SetPubIp(IN CHAR *pcIp);
UINT NAT_Main_GetPubIp();
VOID NAT_Main_SetGateWay(IN CHAR *pcIp);
VOID NAT_Main_SetGateWayMac(IN CHAR *pcMac);
VOID NAT_Main_SetPubMac(IN CHAR *pcMac);

VOID NAT_Main_Show();
VOID NAT_Main_Save(IN HANDLE hFile);

VOID NAT_Main_PktInput(IN MBUF_S *pstMbuf);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__NAT_MAIN_H_*/


