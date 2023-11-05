/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2015-12-2
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "comp/comp_acl.h"
#include "comp/comp_kfapp.h"

#include "wsapp_def.h"
#include "wsapp_gw.h"
#include "wsapp_worker.h"
#include "wsapp_master.h"
#include "wsapp_cfglock.h"
#include "wsapp_func.h"
#include "wsapp_gw_cmd.h"
#include "wsapp_service.h"
#include "wsapp_service_cmd.h"

BS_STATUS WSAPP_Init()
{
    WSAPP_CfgLock_Init();
    WSAPP_GW_Init();
    WSAPP_Service_Init();
    WSAPP_Master_Init();
    WSAPP_Worker_Init();
    return BS_OK;
}


PLUG_API BS_STATUS WSAPP_Save(IN HANDLE hFile)
{
    WSAPP_CfgLock_RLock();
    WSAPP_GwCmd_Save(hFile);
    WSAPP_ServiceCmd_Save(hFile);
    WSAPP_CfgLock_RUnLock();

    return BS_OK;
}

