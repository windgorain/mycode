/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2015-12-1
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "comp/comp_wsapp.h"

#include "wsapp_def.h"
#include "wsapp_worker.h"
#include "wsapp_gw.h"
#include "wsapp_func.h"
#include "wsapp_service.h"
#include "wsapp_service_cfg.h"

static COMP_WSAPP_S g_stWsAppComp;

VOID WSAPP_COMP_Init()
{
    g_stWsAppComp.comp.comp_name = COMP_WSAPP_NAME;
    g_stWsAppComp.pfAddAutoNameService = WSAPP_ServiceCfg_AddAutoNameService;
    g_stWsAppComp.pfDelService = WSAPP_ServiceCfg_Del;
    g_stWsAppComp.pfServiceBindGw = WSAPP_ServiceCfg_ServiceBindGateway;
    g_stWsAppComp.pfServiceBind = WSAPP_ServiceCfg_Bind;
    g_stWsAppComp.pfServiceUnbind = WSAPP_ServiceCfg_UnBind;
    g_stWsAppComp.pfSetDeliverTbl = WSAPP_ServiceCfg_SetDeliverTbl;
    g_stWsAppComp.pfSetServiceUserData = WSAPP_Service_SetUserData;
    g_stWsAppComp.pfGetSerivceUserDataByWsContext = WSAPP_Service_GetUserDataByWsContext;
    g_stWsAppComp.pfSetDocRoot = WSAPP_Service_SetDocRoot;
    g_stWsAppComp.pfSetIndex = WSAPP_Service_SetIndex;
    g_stWsAppComp.pfRegWorkerListener = WSAPP_Worker_RegListener;
    g_stWsAppComp.pfSetListenerData = WSAPP_Worker_SetListenerData;
    g_stWsAppComp.pfGetListnerData = WSAPP_Worker_GetListenerData;

    COMP_Reg(&g_stWsAppComp.comp);
}


