/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2013-11-20
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/ses_utl.h"
#include "utl/mutex_utl.h"
#include "utl/exec_utl.h"
#include "comp/comp_if.h"

#include "../inc/vnets_phy.h"
#include "../inc/vnets_ses.h"
#include "../inc/vnets_master.h"
#include "../inc/vnets_context.h"
#include "../inc/vnets_vpn_link.h"

#define _VNETS_SES_MAX_NUM (1024 * 64)

#define _VNETS_SES_TIME_OUT_TIME  1000  /* 1s */
#define _VNETS_SES_KEEP_ALIVE_IDLE   300
#define _VNETS_SES_KEEP_ALIVE_INTVAL 3
#define _VNETS_SES_KEEP_ALIVE_MAX_TRYS 3

static SES_HANDLE g_hVnetsSesHandle = NULL;
static MUTEX_S g_stVnetsSesMutex; /* 只用于互斥删除和显示.其他不存在问题 */

static BS_STATUS vnets_ses_RecvPkt(IN UINT uiSesID, IN MBUF_S *pstMbuf)
{
    VNETS_Context_SetRecvSesID(pstMbuf, uiSesID);

    return VNETS_VPN_LINK_Input(pstMbuf);
}

static BS_STATUS vnets_ses_SendPkt(IN MBUF_S *pstMbuf, IN VOID *pUserContext)
{
    VNETS_PHY_CONTEXT_S *pstContext = pUserContext;
    VNETS_PHY_CONTEXT_S *pstPhyContext;

    if (NULL == VNETS_Context_AddContextToMbuf(pstMbuf))
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    pstPhyContext = VNETS_Context_GetPhyContext(pstMbuf);

    *pstPhyContext = *pstContext;

    return IFNET_PhyOutput(pstContext->uiIfIndex, pstMbuf);
}

static BS_STATUS vnets_ses_EventNotify(IN UINT uiSesID, IN UINT uiEvent)
{
    switch (uiEvent)
    {
        case SES_EVENT_PEER_CLOSED:
        case SES_EVENT_CONNECT_FAILED:
        {
            SES_Close(g_hVnetsSesHandle, uiSesID);
            break;
        }

        default:
        {
            break;
        }
    }

    return BS_OK;
}

static VOID vnets_ses_PktInput(void *ud)
{
    MBUF_S *pstMbuf = ud;
    
    SES_PktInput(g_hVnetsSesHandle, pstMbuf, VNETS_Context_GetPhyContext(pstMbuf));
}

static VOID vnets_ses_TimeOut(IN HANDLE hTimerHandle, IN USER_HANDLE_S *pstUserHandle)
{
    SES_TimerStep(g_hVnetsSesHandle);
}

BS_STATUS VNETS_SES_Init()
{
    SES_OPT_KEEP_ALIVE_TIME_S stKeepAliveSet;
    
    g_hVnetsSesHandle = SES_CreateInstance(_VNETS_SES_MAX_NUM,
            sizeof(VNETS_PHY_CONTEXT_S),
            0,
            vnets_ses_RecvPkt,
            vnets_ses_SendPkt,
            vnets_ses_EventNotify);
    if (g_hVnetsSesHandle == NULL)
    {
        return BS_ERR;
    }

    stKeepAliveSet.usIdle = _VNETS_SES_KEEP_ALIVE_IDLE;
    stKeepAliveSet.usIntval = _VNETS_SES_KEEP_ALIVE_INTVAL;
    stKeepAliveSet.usMaxProbeCount= _VNETS_SES_KEEP_ALIVE_MAX_TRYS;

    SES_SetDftKeepAlive(g_hVnetsSesHandle, &stKeepAliveSet);

    MUTEX_Init(&g_stVnetsSesMutex);

    VNETS_Master_AddTimer(_VNETS_SES_TIME_OUT_TIME, TRUE, vnets_ses_TimeOut, NULL);

    return BS_OK;
}

BS_STATUS VNETS_SES_RegCloseNotifyEvent(IN PF_SES_CLOSE_NOTIFY_FUNC pfFunc, IN USER_HANDLE_S *pstUserHandle)
{
    return SES_RegCloseNotifyEvent(g_hVnetsSesHandle, pfFunc, pstUserHandle);
}

BS_STATUS VNETS_SES_PktInput(IN UINT ulIfIndex, IN MBUF_S *pstMbuf)
{
    if (BS_OK != VNETS_Master_MsgInput(vnets_ses_PktInput, pstMbuf))
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    return BS_OK;
}

BS_STATUS VNETS_SES_SendPkt(IN UINT uiSesID, IN MBUF_S *pstMbuf)
{
    return SES_SendPkt(g_hVnetsSesHandle, uiSesID, pstMbuf);
}

VOID VNETS_SES_Close(IN UINT uiSesID)
{
    MUTEX_P(&g_stVnetsSesMutex);
    SES_Close(g_hVnetsSesHandle, uiSesID);
    MUTEX_V(&g_stVnetsSesMutex);
}

BS_STATUS VNETS_SES_GetPhyInfo(IN UINT uiSesID, OUT VNETS_PHY_CONTEXT_S *pstPhyInfo)
{
    VNETS_PHY_CONTEXT_S *pstPhyInfoTmp;
    BS_STATUS eRet = BS_NOT_FOUND;
    
    MUTEX_P(&g_stVnetsSesMutex);
    pstPhyInfoTmp = SES_GetUsrContext(g_hVnetsSesHandle, uiSesID);
    if (NULL != pstPhyInfoTmp)
    {
        *pstPhyInfo = *pstPhyInfoTmp;
        eRet = BS_OK;
    }
    MUTEX_V(&g_stVnetsSesMutex);

    return eRet;
}

UINT VNETS_SES_GetIfIndex(IN UINT uiSesID)
{
    VNETS_PHY_CONTEXT_S *pstContext;
    UINT uiIfIndex;

    pstContext = SES_GetUsrContext(g_hVnetsSesHandle, uiSesID);
    if (NULL == pstContext)
    {
        return 0;
    }

    uiIfIndex = pstContext->uiIfIndex;

    return uiIfIndex;
}

BS_STATUS VNETS_SES_SetProperty(IN UINT uiSesID, IN UINT uiIndex, IN HANDLE hValue)
{
    return SES_SetProperty(g_hVnetsSesHandle, uiSesID, uiIndex, hValue);
}

HANDLE VNETS_SES_GetProperty(IN UINT uiSesID, IN UINT uiIndex)
{
    return SES_GetProperty(g_hVnetsSesHandle, uiSesID, uiIndex);
}

static BS_WALK_RET_E vnets_ses_show(IN UINT uiSesID, IN HANDLE hUserHandle)
{
    VNETS_PHY_CONTEXT_S *pstPhyContext;
    UINT uiPeerIP;
    USHORT usPeerPort;

    pstPhyContext = SES_GetUsrContext(g_hVnetsSesHandle, uiSesID);
    if (NULL == pstPhyContext)
    {
        return BS_WALK_CONTINUE;
    }

    uiPeerIP = pstPhyContext->unPhyContext.stUdpPhyContext.uiPeerIp;
    usPeerPort = pstPhyContext->unPhyContext.stUdpPhyContext.usPeerPort;

    EXEC_OutInfo(" %-8x %-9x %-15pI4 %-8d %s \r\n",
        uiSesID,
        SES_GetPeerSESID(g_hVnetsSesHandle, uiSesID),
        &uiPeerIP, ntohs(usPeerPort),
        SES_GetStatusString(SES_GetStatus(g_hVnetsSesHandle, uiSesID)));

    

    return BS_WALK_CONTINUE;
}

/* show session */
PLUG_API BS_STATUS VNETS_SES_Show(IN UINT ulArgc, IN CHAR **argv)
{
    EXEC_OutInfo(" SESID    PeerSESID PeerIP          PeerPort Status\r\n"
                            "------------------------------------------------------\r\n");

    MUTEX_P(&g_stVnetsSesMutex);
    SES_Walk(g_hVnetsSesHandle, vnets_ses_show, NULL);
    MUTEX_V(&g_stVnetsSesMutex);

    EXEC_OutString("\r\n");

    return BS_OK;
}

/* debug session protodol packet*/
PLUG_API BS_STATUS VNETS_SES_DebugProtocolPacket(IN UINT ulArgc, IN CHAR **argv)
{
    SES_AddDbgFlag(g_hVnetsSesHandle, SES_DBG_FLAG_PROTOCOL_PKT);
    return BS_OK;
}

/* no debug session protodol packet*/
PLUG_API BS_STATUS VNETS_SES_NoDebugProtocolPacket(IN UINT ulArgc, IN CHAR **argv)
{
    SES_ClrDbgFlag(g_hVnetsSesHandle, SES_DBG_FLAG_PROTOCOL_PKT);
    return BS_OK;
}

/* debug session data packet*/
PLUG_API BS_STATUS VNETS_SES_DebugDataPacket(IN UINT ulArgc, IN CHAR **argv)
{
    SES_AddDbgFlag(g_hVnetsSesHandle, SES_DBG_FLAG_DATA_PKT);
    return BS_OK;
}

/* no debug session data packet*/
PLUG_API BS_STATUS VNETS_SES_NoDebugDataPacket(IN UINT ulArgc, IN CHAR **argv)
{
    SES_ClrDbgFlag(g_hVnetsSesHandle, SES_DBG_FLAG_DATA_PKT);
    return BS_OK;
}

BS_STATUS VNETS_SES_NoDebugAll()
{
    SES_ClrDbgFlag(g_hVnetsSesHandle, 0xffffffff);
	return BS_OK;
}

