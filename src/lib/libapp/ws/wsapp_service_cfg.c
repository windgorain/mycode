/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2015-12-3
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
    
#include "utl/txt_utl.h"
#include "utl/ws_utl.h"
#include "utl/nap_utl.h"

#include "wsapp_service_cfg.h"
#include "wsapp_service.h"
#include "wsapp_cfglock.h"

CHAR * WSAPP_AddAutoNameService(IN UINT uiFlag)
{
    CHAR *pcName;

    WSAPP_CfgLock_WLock();
    pcName = WSAPP_Service_AddAutoNameService(uiFlag);
    WSAPP_CfgLock_WUnLock();

    return pcName;
}

BS_STATUS WSAPP_DelService(IN CHAR *pcServiceName)
{
    BS_STATUS eRet;

    WSAPP_CfgLock_WLock();
    eRet = WSAPP_Service_Del(pcServiceName);
    WSAPP_CfgLock_WUnLock();

    return eRet;
}

BS_STATUS WSAPP_BindGw
(
    IN CHAR *pcService,
    IN CHAR *pcGateWay,
    IN CHAR *pcVHost,
    IN CHAR *pcDomain
)
{
    BS_STATUS eRet;

    WSAPP_CfgLock_WLock();
    eRet = WSAPP_Service_BindGateway(pcService, pcGateWay, pcVHost, pcDomain);
    WSAPP_CfgLock_WUnLock();

    return eRet;
}

BS_STATUS WSAPP_BindService(IN CHAR *pcService)
{
    BS_STATUS eRet;

    WSAPP_CfgLock_WLock();
    eRet = WSAPP_Service_Bind(pcService);
    WSAPP_CfgLock_WUnLock();

    return eRet;
}

BS_STATUS WSAPP_UnBindService(IN CHAR *pcService)
{
    WSAPP_CfgLock_WLock();
    WSAPP_Service_UnBind(pcService);
    WSAPP_CfgLock_WUnLock();

	return BS_OK;
}

BS_STATUS WSAPP_SetDeliverTbl(IN CHAR *pcService, IN WS_DELIVER_TBL_HANDLE hDeliverTbl)
{
    BS_STATUS eRet;

    WSAPP_CfgLock_WLock();
    eRet = WSAPP_Service_SetDeliverTbl(pcService, hDeliverTbl);
    WSAPP_CfgLock_WUnLock();

    return eRet;
}
