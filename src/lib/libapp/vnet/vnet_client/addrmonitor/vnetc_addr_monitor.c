/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-11-3
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/ob_chain.h"

#include "../inc/vnetc_master.h"
#include "../inc/vnetc_vnic_phy.h"
#include "../inc/vnetc_addr_monitor.h"

#define _VNETC_P_ADDR_MONITOR_TIMEOUT_TIME 1000  

static VCLOCK_HANDLE g_hVnetcAddrMonitorVclock = NULL;
static UINT g_uiVnetcAddrMonitorIp = 0;     
static UINT g_uiVnetcAddrMonitorMask = 0;   
static OB_CHAIN_S g_stVnetcAddrMonitorObChain = OB_CHAIN_HEAD_INIT_VALUE(&g_stVnetcAddrMonitorObChain);

static VOID vnetc_addrmonitor_TimeOut(IN HANDLE hTimerHandle, IN USER_HANDLE_S *pstUserHandle)
{
    UINT uiIp;
    UINT uiMask;
    
    if (BS_OK != VNETC_VNIC_PHY_GetIp(&uiIp, &uiMask))
    {
        return;
    }

    if ((uiIp == g_uiVnetcAddrMonitorIp) && (uiMask == g_uiVnetcAddrMonitorMask))
    {
        return;
    }

    g_uiVnetcAddrMonitorIp = uiIp;
    g_uiVnetcAddrMonitorMask = uiMask;

    OB_CHAIN_NOTIFY(&g_stVnetcAddrMonitorObChain, PF_VNETC_AddrMonitor_Notify_Func);
}


BS_STATUS VNETC_AddrMonitor_Init()
{
    g_hVnetcAddrMonitorVclock =
        VNETC_Master_AddTimer(_VNETC_P_ADDR_MONITOR_TIMEOUT_TIME, TIMER_FLAG_CYCLE, vnetc_addrmonitor_TimeOut, NULL);

    return BS_OK;
}

UINT VNETC_AddrMonitor_GetIP()
{
    return g_uiVnetcAddrMonitorIp;
}

UINT VNETC_AddrMonitor_GetMask()
{
    return g_uiVnetcAddrMonitorMask;
}

BS_STATUS VNETC_AddrMonitor_RegNotify
(
    IN PF_VNETC_AddrMonitor_Notify_Func pfFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    return OB_CHAIN_Add(&g_stVnetcAddrMonitorObChain, pfFunc, pstUserHandle);
}

