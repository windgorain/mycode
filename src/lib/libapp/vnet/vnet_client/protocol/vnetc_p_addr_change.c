
#include "bs.h"

#include "utl/my_ip_helper.h"

#include "../inc/vnetc_conf.h"
#include "../inc/vnetc_protocol.h"
#include "../inc/vnetc_master.h"
#include "../inc/vnetc_fsm.h"
#include "../inc/vnetc_p_addr_change.h"
#include "../inc/vnetc_addr_monitor.h"
#include "../inc/vnetc_p_nodeinfo.h"

#define VNETC_P_ADDR_ROUTE_TRY_MAX_TIME 60

static BOOL_T g_bVnetcPAddrChangeStarted = FALSE;
static UINT g_uiVnetcPAddrRouteTryTime = 0; /* 尝试检测虚拟缺省路由的次数 */
static VCLOCK_HANDLE g_hVnetcPAddrRouteTimer = NULL;


/* 判断是否我们的默认路由 */
static BOOL_T vnetc_p_addrchange_IsVnetDftRoute
(
    IN UINT uiIP, /* 网络序 */
    IN UINT uiMask, /* 网络序 */
    IN MIB_IPFORWARDROW *pstRoute
)
{
    UINT uiNet = uiIP & uiMask;
    UINT uiRouteDest = pstRoute->dwForwardDest & pstRoute->dwForwardMask;

    if ((pstRoute->dwForwardDest != 0) || (pstRoute->dwForwardMask != 0))
    {
        return FALSE; //不是默认路由
    }

    if ((pstRoute->dwForwardNextHop & uiMask) != uiNet)
    {
        return FALSE; //下一跳地址不在虚拟网段范围内,不是我们的默认路由
    }

    return TRUE;
}

static BS_STATUS vnetc_p_addrchange_TryModifyRoute()
{
    UINT uiIp;
    UINT uiMask;
    MIB_IPFORWARDROW *pstIpRoute;
    MIB_IPFORWARDROW *pstIpRouteFound = NULL;
    MIB_IPFORWARDTABLE *pstRouteTbl;
    BS_STATUS eRet = BS_NOT_FOUND;

    uiIp = VNETC_AddrMonitor_GetIP();
    uiMask = VNETC_AddrMonitor_GetMask();

    pstRouteTbl = My_IP_Helper_GetRouteTbl();
    if (NULL == pstRouteTbl)
    {
        return BS_NO_MEMORY;
    }

    MY_IP_HELPER_SCAN_ROUTE_TBL_START(pstRouteTbl, pstIpRoute)
    {
        if (TRUE == vnetc_p_addrchange_IsVnetDftRoute(uiIp, uiMask, pstIpRoute))
        {
            pstIpRouteFound = pstIpRoute;
            break;
        }
    }MY_IP_HELPER_SCAN_ROUTE_TBL_END();

    if (NULL != pstIpRouteFound)
    {
        eRet = BS_OK;
        pstIpRouteFound->dwForwardMetric1 = 1000;
        My_IP_Helper_ModifyRoute(pstIpRouteFound);
    }

    My_IP_Helper_FreeRouteTbl(pstRouteTbl);

    return eRet;
}


/* 将虚拟网卡的gateway路由修改为低优先级,以免影响用户的上网 */
static VOID vnetc_p_addrchange_ModifyRoute(IN HANDLE hTimerHandle, IN USER_HANDLE_S *pstUserHandle)
{
    BS_STATUS eRet;
    
    eRet = vnetc_p_addrchange_TryModifyRoute();

    g_uiVnetcPAddrRouteTryTime ++;

    if ((BS_OK == eRet) || (g_uiVnetcPAddrRouteTryTime > VNETC_P_ADDR_ROUTE_TRY_MAX_TIME))
    {
        VNETC_Master_DelTimer(g_hVnetcPAddrRouteTimer);
        g_hVnetcPAddrRouteTimer = NULL;
    }

    return;
}

static VOID vnetc_p_addrchange_AddrMonitorNotify(IN USER_HANDLE_S *pstUserHandle)
{
    if (g_bVnetcPAddrChangeStarted == TRUE)
    {
        g_uiVnetcPAddrRouteTryTime = 0;
        g_hVnetcPAddrRouteTimer = VNETC_Master_AddTimer(1000, TIMER_FLAG_CYCLE, vnetc_p_addrchange_ModifyRoute, NULL);
        VNETC_P_NodeInfo_SendInfo();
    }
}

static VOID vnetc_p_addrchange_Start()
{
    g_bVnetcPAddrChangeStarted = TRUE;
	VNETC_P_NodeInfo_SendInfo();

    return;
}

static VOID vnetc_p_addrchange_StateChange(IN FSM_S *pstFsm, IN UINT uiOldState, IN UINT uiNewState, IN USER_HANDLE_S *pstUserHandle)
{
    if (uiNewState == VNETC_FSM_STATE_RUNNING)
    {
        vnetc_p_addrchange_Start();
    }
    else
    {
        VNETC_P_AddrChange_Stop();
    }
}

VOID VNETC_P_AddrChange_Stop()
{
    g_bVnetcPAddrChangeStarted = FALSE;
}

BS_STATUS VNETC_P_AddrChange_Init()
{
    VNETC_FSM_RegStateListener(vnetc_p_addrchange_StateChange, NULL);

    return VNETC_AddrMonitor_RegNotify(vnetc_p_addrchange_AddrMonitorNotify, NULL);
}

