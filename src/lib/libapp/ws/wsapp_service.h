/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2015-12-3
* Description: 
* History:     
******************************************************************************/

#ifndef __WSAPP_SERVICE_H_
#define __WSAPP_SERVICE_H_

#include "wsapp_service_cfg.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

BS_STATUS WSAPP_Service_Init();
CHAR * WSAPP_Service_GetName(IN HANDLE hService);
/* 创建一个自动命名的Service, 并返回其名字 */
CHAR * WSAPP_Service_AddAutoNameService(IN UINT uiFlag);
WSAPP_SERVICE_S * WSAPP_Service_Add(IN CHAR *pcServiceName);
BS_STATUS WSAPP_Service_Del(IN CHAR *pcServiceName);
BS_STATUS WSAPP_Service_SetWebCenterOpt(IN CHAR *pcServiceName, IN CHAR *pcOpt);
BS_STATUS WSAPP_Service_ClrWebCenterOpt(IN CHAR *pcServiceName, IN CHAR *pcOpt);
BS_STATUS WSAPP_Service_SetDescription(IN CHAR *pcServiceName, IN CHAR *pcDesc);
BS_STATUS WSAPP_Service_BindGateway
(
    IN CHAR *pcService,
    IN CHAR *pcGateWay,
    IN CHAR *pcVHost,
    IN CHAR *pcDomain
);
BS_STATUS WSAPP_Service_NoBindGateway
(
    IN CHAR *pcService,
    IN CHAR *pcGateWay,
    IN CHAR *pcVHost,
    IN CHAR *pcDomain
);
BS_STATUS WSAPP_Service_Start(IN CHAR *pcServiceName);
BS_STATUS WSAPP_Service_Stop(IN CHAR *pcServiceName);
WSAPP_SERVICE_S * WSAPP_Service_GetByID(IN UINT uiServiceID);
BOOL_T WSAPP_Service_IsWebCenterOptHide(IN WSAPP_SERVICE_S *pstService);
BOOL_T WSAPP_Service_IsWebCenterOptReadonly(IN WSAPP_SERVICE_S *pstService);
CHAR * WSAPP_Service_GetDescription(IN WSAPP_SERVICE_S *pstService);
UINT WSAPP_Service_GetNextID(IN UINT uiCurId);
CHAR * WSAPP_Service_GetNameByID(IN UINT uiServiceID);
BS_STATUS WSAPP_Service_Bind(IN CHAR *pcService);
VOID WSAPP_Service_UnBind(IN CHAR *pcService);
WSAPP_SERVICE_HANDLE WSAPP_Service_GetByName(IN CHAR *pcService);
BS_STATUS WSAPP_Service_SetDeliverTbl(IN CHAR *pcService, IN WS_DELIVER_TBL_HANDLE hDeliverTbl);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__WSAPP_SERVICE_H_*/


