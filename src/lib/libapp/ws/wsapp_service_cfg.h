/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2015-12-3
* Description: 
* History:     
******************************************************************************/

#ifndef __WSAPP_SERVICE_CFG_H_
#define __WSAPP_SERVICE_CFG_H_

#include "utl/file_utl.h"
#include "utl/ws_utl.h"
#include "comp/comp_wsapp.h"
#include "wsapp_def.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define WSAPP_SERVICE_MAX_BIND_NUM 10  /* 一个Service最多绑定多少个GateWay */

typedef struct
{
    CHAR szBindGwName[WSAPP_GW_NAME_LEN + 1];
    CHAR szVHostName[WS_VHOST_MAX_LEN + 1];
    CHAR szDomain[WS_DOMAIN_MAX_LEN + 1];
    WS_CONTEXT_HANDLE hWsContext;
}WSAPP_SERVICE_BIND_INFO_S;


typedef struct
{
    /* 配置相关项 */
    WSAPP_SERVICE_BIND_INFO_S astBindGateWay[WSAPP_SERVICE_MAX_BIND_NUM];
    BOOL_T bStart;

    /* 路径 */
    CHAR szRootPath[FILE_MAX_PATH_LEN + 1];
    CHAR szIndex[WS_CONTEXT_MAX_INDEX_LEN + 1];

    /* 运行相关项 */
    UINT uiFlag;
    UINT64 ulUserData;
    WS_DELIVER_TBL_HANDLE hDeliverTbl;
}WSAPP_SERVICE_S;


CHAR * WSAPP_ServiceCfg_AddAutoNameService(IN UINT uiFlag);
BS_STATUS WSAPP_ServiceCfg_Del(IN CHAR *pcServiceName);
BS_STATUS WSAPP_ServiceCfg_ServiceBindGateway
(
    IN CHAR *pcService,
    IN CHAR *pcGateWay,
    IN CHAR *pcVHost,
    IN CHAR *pcDomain
);
BS_STATUS WSAPP_ServiceCfg_Bind(IN CHAR *pcService);
BS_STATUS WSAPP_ServiceCfg_UnBind(IN CHAR *pcService);
BS_STATUS WSAPP_ServiceCfg_SetDeliverTbl(IN CHAR *pcService, IN WS_DELIVER_TBL_HANDLE hDeliverTbl);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__WSAPP_SERVICE_CFG_H_*/


