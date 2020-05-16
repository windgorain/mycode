/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-6-22
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "../h/svpnc_conf.h"
#include "../h/svpnc_func.h"
#include "../h/svpnc_log.h"

BS_STATUS SVPNC_Init()
{
    SVPNC_Log_Init();
    SVPNC_CMD_Init();
    SVPNC_SslCtx_Init();
    SVPNC_TrRes_Init();
    SVPNC_TR_MainInit();
    SVPNC_TRService_Init();
    SVPNC_KF_Init();

	return BS_OK;
}

