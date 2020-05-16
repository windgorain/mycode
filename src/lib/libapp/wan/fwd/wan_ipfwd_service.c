/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-3-20
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/sif_utl.h"
#include "utl/ipfwd_service.h"
#include "comp/comp_wan.h"

#include "../h/wan_ifnet.h"

static IPFWD_SERVICE_HANDLE g_hWanIpFwdService;

BS_STATUS WAN_IpFwdService_Init()
{
    g_hWanIpFwdService = IPFWD_Service_Create();
    if (NULL == g_hWanIpFwdService)
    {
        return BS_ERR;
    }

    return BS_OK;
}

/* 所有的注册必须要在系统正式运行前注册完成.
 如果某个系统不需要处理,到自己里面去判断,
 以免在注册过程中同时报文处理导致死机 */
BS_STATUS WAN_IpFwdService_Reg
(
    IN IPFWD_SERVICE_PHASE_E ePhase,
    IN UINT uiOrder,
    IN CHAR *pcName,
    IN PF_IPFWD_SERVICE_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    return IPFWD_Service_Reg(g_hWanIpFwdService, ePhase, uiOrder, pcName, pfFunc, pstUserHandle);
}

IPFWD_SERVICE_RET_E WAN_IpFwdService_Process
(
    IN IPFWD_SERVICE_PHASE_E ePhase,
    IN IP_HEAD_S *pstIpHead,
    IN MBUF_S *pstMbuf
)
{
    return IPFWD_Service_Handle(g_hWanIpFwdService, ePhase, pstIpHead, pstMbuf);
}


