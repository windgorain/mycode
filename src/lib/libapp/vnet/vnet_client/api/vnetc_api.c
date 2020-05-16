
#include "bs.h"

#include "../../inc/vnet_conf.h"

#include "../inc/vnetc_conf.h"
#include "../inc/vnetc_user_status.h"
#include "../inc/vnetc_addr_monitor.h"
#include "../inc/vnetc_api.h"

static VNETC_API_TBL_S g_stVnetcApiTbl =
{
    .comp.comp_name = VNETC_COMP_NAME,
    .pfGetVersion = VNETC_GetVersion,
    .pfSetServer = VNETC_SetServer,
    .pfSetUserName = VNETC_SetUserName,
    .pfSetPassword = VNETC_SetUserPasswdSimple,
    .pfSetDomain = VNETC_SetDomainName,
    .pfSetDescription = VNETC_SetDescription,
    .pfStart = VNETC_Start,
    .pfStop = VNETC_Stop,
    .pfGetUserStatus = VNETC_User_GetStatus,
    .pfGetUserStatusString = VNETC_User_GetStatusString,
    .pfGetUserReason = VNETC_User_GetReason,
    .pfGetUserReasonString = VNETC_User_GetReasonString,
    .pfRegAddrMonitorNotify = VNETC_AddrMonitor_RegNotify,
    .pfAddrMonitorGetIP = VNETC_AddrMonitor_GetIP,
    .pfAddrMonitorGetMask = VNETC_AddrMonitor_GetMask
};

PLUG_API VNETC_API_TBL_S * VNETC_API_Load()
{
    return &g_stVnetcApiTbl;
}

BS_STATUS VNETC_API_Init()
{
    COMP_Reg(&g_stVnetcApiTbl.comp);
    return BS_OK;
}

