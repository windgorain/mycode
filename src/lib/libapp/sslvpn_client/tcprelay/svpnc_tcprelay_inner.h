/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-6-30
* Description: 
* History:     
******************************************************************************/

#ifndef __SVPNC_TCPRELAY_INNER_H_
#define __SVPNC_TCPRELAY_INNER_H_

#include "utl/conn_utl.h"
#include "utl/vbuf_utl.h"
#include "utl/fsm_utl.h"
#include "comp/comp_wsapp.h"

#ifdef __cplusplus
    extern "C" {
#endif 


typedef struct
{
    FSM_S stFsm;
    UINT uiServerIP;        
    USHORT usServerPort;    
    CONN_HANDLE hDownConn;
    CONN_HANDLE hUpConn;
    VBUF_S stDownVBuf;   
    VBUF_S stUpVBuf;     
    HTTP_HEAD_PARSER hHttpHeadParser;
}SVPNC_TCPRELAY_NODE_S;

BS_STATUS SVPNC_TrRes_Init();
BS_STATUS SVPNC_TrRes_Add(IN CHAR *pcName, IN CHAR *pcServerAddress);
VOID SVPNC_TR_Write2File();
BOOL_T SVPNC_TR_IsPermit(IN UINT uiIp, IN USHORT usPort);

SVPNC_TCPRELAY_NODE_S * SVPNC_TRNode_New();
VOID SVPNC_TRNode_Free(IN SVPNC_TCPRELAY_NODE_S *pstNode);
MYPOLL_HANDLE SVPNC_TR_GetMyPoller();

#ifdef __cplusplus
    }
#endif 

#endif 


