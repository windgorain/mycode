/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-11-30
* Description: 
* History:     
******************************************************************************/

#ifndef __WSAPP_GW_H_
#define __WSAPP_GW_H_

#include "utl/file_utl.h"
#include "utl/ws_utl.h"
#include "utl/conn_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef struct
{
    UCHAR bStart:1;
    UCHAR bIsSSL:1;
    UINT uiRefCount;
    INT iListenSocket;
    WS_HANDLE hWsHandle;
    UINT64 uiIpAclListID;
    VOID *apSslCtx[WSAPP_WROKER_NUM];
}WSAPP_GW_S;


typedef VOID (*PF_WSAPP_GW_WALK_FUNC)(IN WSAPP_GW_S *pstService, IN HANDLE hUserContext);

BS_STATUS WSAPP_GW_Init();
WSAPP_GW_S * WSAPP_GW_Add(IN CHAR *pcGwName);
BS_STATUS WSAPP_GW_Del(IN CHAR *pcGwName);
BOOL_T WSAPP_GW_IsExist(IN CHAR *pcServiceName);
BS_STATUS WSAPP_GW_SetWebCenterOpt(IN CHAR *pcGwName, IN CHAR *pcOpt);
BS_STATUS WSAPP_GW_ClrWebCenterOpt(IN CHAR *pcGwName, IN CHAR *pcOpt);
BS_STATUS WSAPP_GW_SetDescription(IN CHAR *pcGwName, IN CHAR *pcDesc);
BS_STATUS WSAPP_GW_SetType(IN CHAR *pcGwName, IN CHAR *pcType);
BS_STATUS WSAPP_GW_SetSslParam(IN CHAR *pcServiceName, IN CHAR *pcCACert, IN CHAR *pcLocalCert, IN CHAR *pcKeyFile);
BS_STATUS WSAPP_GW_SetIP(IN CHAR *pcGwName, IN CHAR *pcIP);
BS_STATUS WSAPP_GW_SetPort(IN CHAR *pcGwName, IN CHAR *pcPort);
BS_STATUS WSAPP_GW_RefIpAcl(IN CHAR *pcGwName, IN CHAR *pcIpAclListName);
BS_STATUS WSAPP_GW_NoRefIpAcl(IN CHAR *pcGwName);
BS_STATUS WSAPP_GW_Start(IN CHAR *pcServiceName);
BS_STATUS WSAPP_GW_Stop(IN CHAR *pcServiceName);
VOID WSAPP_GW_SetWsDebugFlag(IN CHAR *pcGwName, IN CHAR *pcFlagName);
VOID WSAPP_GW_ClrWsDebugFlag(IN CHAR *pcGwName, IN CHAR *pcFlagName);
VOID WSAPP_GW_Walk(IN PF_WSAPP_GW_WALK_FUNC pfFunc, IN HANDLE hUserContext);
VOID * WSAPP_GW_GetSslCtx(IN UINT uiServiceID, IN UINT uiWorkerID);
BS_STATUS WSAPP_GW_NewConn(IN UINT uiGwID, IN CONN_HANDLE hConn);
WSAPP_GW_S * WSAPP_GW_GetByID(IN UINT uiServiceID);
UINT WSAPP_GW_GetID(IN WSAPP_GW_S *pstGW);
WS_CONTEXT_HANDLE WSAPP_GW_AddService
(
    IN CHAR *pcServiceName,
    IN CHAR *pcVHost,   /* 可以为NULL, 表示使用缺省 */
    IN CHAR *pcDomain   /* 可以为NULL, 表示独占 */
);
VOID WSAPP_GW_DelService(IN CHAR *pcGwName, IN WS_CONTEXT_HANDLE hContext);
CHAR * WSAPP_GW_GetName(IN WSAPP_GW_S *pstGW);
BOOL_T WSAPP_GW_IsWebCenterOptHide(IN WSAPP_GW_S *pstGW);
BOOL_T WSAPP_GW_IsWebCenterOptReadonly(IN WSAPP_GW_S *pstGW);
CHAR * WSAPP_GW_GetDesc(IN WSAPP_GW_S *pstGW);
CHAR * WSAPP_GW_GetParamCaCert(IN WSAPP_GW_S *pstGW);
CHAR * WSAPP_GW_GetParamLocalCert(IN WSAPP_GW_S *pstGW);
CHAR * WSAPP_GW_GetParamKeyFile(IN WSAPP_GW_S *pstGW);
CHAR * WSAPP_GW_GetIP(IN WSAPP_GW_S *pstGW);
CHAR * WSAPP_GW_GetPort(IN WSAPP_GW_S *pstGW);
CHAR * WSAPP_GW_GetIpAclList(IN WSAPP_GW_S *pstGW);
BOOL_T WSAPP_GW_IsFilterPermit(IN UINT uiGwID, IN INT iSocketID);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__WSAPP_GW_H_*/


