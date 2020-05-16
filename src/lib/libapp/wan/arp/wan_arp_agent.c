/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2017-4-28
* Description: ARP代理, 当在接口上使能了此功能后,接口收到的ARP请求后会进行代理应答.
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/socket_utl.h"
#include "utl/exec_utl.h"
#include "utl/sif_utl.h"
#include "utl/ip_list.h"
#include "utl/bit_opt.h"
#include "utl/mutex_utl.h"
#include "comp/comp_if.h"

#include "../h/wan_ifnet.h"
#include "../h/wan_arp_agent.h"

#define _WAN_ARP_AGENT_ON 0x1

typedef struct
{
    RCU_NODE_S stRcuNode;
    UINT uiFlag;
    IPLIST_S stIpList;
}WAN_ARP_AGENT_CTRL_S;

static UINT g_uiWanArpAgentIfUserDataIndex = 0;
static MUTEX_S g_stWanArpAgentMutex;

static VOID _wan_arpagent_InterfaceSaveInner(IN IF_INDEX ifIndex, IN HANDLE hFile)
{
    WAN_ARP_AGENT_CTRL_S *pstCtrl;
    UINT uiBeginIP, uiEndIP;

    if ((BS_OK != CompIf_GetUserData(ifIndex, g_uiWanArpAgentIfUserDataIndex, &pstCtrl))
        || (pstCtrl == NULL))
    {
        return;
    }

    IPLIST_SCAN_BEGIN(&pstCtrl->stIpList, uiBeginIP, uiEndIP)
    {
        uiBeginIP = htonl(uiBeginIP);
        uiEndIP = htonl(uiEndIP);
        CMD_EXP_OutputCmd(hFile, "arp-agent ippool %pI4 %pI4", &uiBeginIP, &uiEndIP);
    }IPLIST_SCAN_END();

    if (pstCtrl->uiFlag & _WAN_ARP_AGENT_ON)
    {
        CMD_EXP_OutputCmd(hFile, "arp-agent enable");
    }
    
    return;
}

static VOID _wan_arpagent_InterfaceSave(IN IF_INDEX ifIndex, IN HANDLE hFile)
{
    UINT uiPhase;

    uiPhase = RcuBs_Lock();
    _wan_arpagent_InterfaceSaveInner(ifIndex, hFile);
    RcuBs_UnLock(uiPhase);

    return;
}

static WAN_ARP_AGENT_CTRL_S * _wan_arpagent_GetCtrlWithCreate(IN IF_INDEX ifIndex)
{
    WAN_ARP_AGENT_CTRL_S *pstCtrl;
    
    MUTEX_P(&g_stWanArpAgentMutex);
    if ((BS_OK != CompIf_GetUserData(ifIndex, g_uiWanArpAgentIfUserDataIndex, &pstCtrl)) || (NULL == pstCtrl))
    {
        pstCtrl = MEM_ZMalloc(sizeof(WAN_ARP_AGENT_CTRL_S));
        IPList_Init(&pstCtrl->stIpList);
        if (BS_OK != (CompIf_SetUserData(ifIndex, g_uiWanArpAgentIfUserDataIndex, pstCtrl)))
        {
            MEM_Free(pstCtrl);
            pstCtrl = NULL;
        }
    }    
    MUTEX_V(&g_stWanArpAgentMutex);

    return pstCtrl;
}

static VOID _mem_RcuFreeCallBack(IN VOID *pstRcuNode)
{
    WAN_ARP_AGENT_CTRL_S *pstCtrl = pstRcuNode;

    IPList_Finit(&pstCtrl->stIpList);
    MEM_Free(pstCtrl);
}

static VOID _wan_arp_IfDeleteEvent(IN IF_INDEX ifIndex)
{
    WAN_ARP_AGENT_CTRL_S *pstCtrl;

    if ((BS_OK != CompIf_GetUserData(ifIndex, g_uiWanArpAgentIfUserDataIndex, (HANDLE*)&pstCtrl))
        || (NULL == pstCtrl))
    {
        return;
    }

    CompIf_SetUserData(ifIndex, g_uiWanArpAgentIfUserDataIndex, NULL);

    RcuBs_Free((VOID*)pstCtrl, _mem_RcuFreeCallBack);

    return;
}

static BS_STATUS _wan_arpagent_IfEvent(IN IF_INDEX ifIndex, IN UINT uiEvent, IN USER_HANDLE_S *pstUserHandle)
{
    switch (uiEvent)
    {
        case IF_EVENT_DELETE:
        {
            _wan_arp_IfDeleteEvent(ifIndex);
            break;
        }

        default:
        {
            break;
        }
    }

    return BS_OK;
}

BS_STATUS WAN_ArpAgent_Init()
{
    MUTEX_Init(&g_stWanArpAgentMutex);
    g_uiWanArpAgentIfUserDataIndex = CompIf_AllocUserDataIndex();
    CompIf_RegSave(_wan_arpagent_InterfaceSave);
    CompIf_RegEvent(_wan_arpagent_IfEvent, NULL);

    return BS_OK;
}

static BOOL_T _wan_arpagent_IsAgentOn(IN IF_INDEX ifIndex, IN UINT uiIP/* 主机序 */)
{
    WAN_ARP_AGENT_CTRL_S *pstCtrl;

    if ((BS_OK != CompIf_GetUserData(ifIndex, g_uiWanArpAgentIfUserDataIndex, &pstCtrl))
        || (pstCtrl == NULL))
    {
        return FALSE;
    }

    if ((pstCtrl->uiFlag & _WAN_ARP_AGENT_ON) == 0)
    {
        return FALSE;
    }

    if (IPList_IsIPInTheList(&pstCtrl->stIpList, uiIP))
    {
        return TRUE;
    }

    return FALSE;
}

BOOL_T WAN_ArpAgent_IsAgentOn(IN IF_INDEX ifIndex, IN UINT uiIP/* 主机序 */)
{
    BOOL_T bRet;
    UINT uiPhase;

    uiPhase = RcuBs_Lock();
    bRet = _wan_arpagent_IsAgentOn(ifIndex, uiIP);
    RcuBs_UnLock(uiPhase);

    return bRet;
}

/*
Interface 视图下:
[no] arp-agent ippool begin end
*/
PLUG_API BS_STATUS WAN_ArpAgent_IpPool
(
    IN UINT ulArgc,
    IN CHAR **argv,
    IN VOID *pEnv
)
{
    IF_INDEX ifIndex;
    CHAR *pcStart, *pcEnd;
    WAN_ARP_AGENT_CTRL_S *pstCtrl;
    UINT uiStart;
    UINT uiEnd;
    UINT uiTmp;
    UINT uiPhase;
    BS_STATUS eRet = BS_OK;

    ifIndex = WAN_IF_GetIfIndexByEnv(pEnv);
    if (ifIndex == IF_INVALID_INDEX)
    {
        EXEC_OutString(" Interface is not exist.\r\n");
        return BS_ERR;
    }

    uiPhase = RcuBs_Lock();

    pstCtrl = _wan_arpagent_GetCtrlWithCreate(ifIndex);
    if (NULL == pstCtrl)
    {
        EXEC_OutString(" error.\r\n");
        eRet = BS_ERR;
    }
    else
    {
        if (argv[0][0] == 'n')
        {
            pcStart = argv[3];
            pcEnd = argv[4];
        }
        else
        {
            pcStart = argv[2];
            pcEnd = argv[3];
        }

        uiStart = Socket_NameToIpHost(pcStart);
        uiEnd = Socket_NameToIpHost(pcEnd);

        if (uiStart > uiEnd)
        {
            uiTmp = uiStart;
            uiStart = uiEnd;
            uiEnd = uiTmp;
        }

        if (argv[0][0] != 'n')
        {
            IPList_AddRange(&pstCtrl->stIpList, uiStart, uiEnd);
        }
        else
        {
            IPList_DelRange(&pstCtrl->stIpList, uiStart, uiEnd);
        }
    }

    RcuBs_UnLock(uiPhase);

    return eRet;
}

/*
Interface 视图下:
[no] arp-agent enable
*/
PLUG_API BS_STATUS WAN_ArpAgent_Enable
(
    IN UINT ulArgc,
    IN CHAR **argv,
    IN VOID *pEnv
)
{
    IF_INDEX ifIndex;
    UINT uiPhase;
    BS_STATUS eRet = BS_OK;
    WAN_ARP_AGENT_CTRL_S *pstCtrl;

    ifIndex = WAN_IF_GetIfIndexByEnv(pEnv);
    if (ifIndex == IF_INVALID_INDEX)
    {
        EXEC_OutString(" Interface is not exist.\r\n");
        return BS_ERR;
    }

    uiPhase = RcuBs_Lock();

    pstCtrl = _wan_arpagent_GetCtrlWithCreate(ifIndex);
    if (NULL == pstCtrl)
    {
        EXEC_OutString(" error.\r\n");
        eRet = BS_ERR;
    }
    else
    {
        /* no arp-agent enable */
        if (argv[0][0] != 'n')
        {
            BIT_SET(pstCtrl->uiFlag, _WAN_ARP_AGENT_ON);
        }
        else
        {
            BIT_CLR(pstCtrl->uiFlag, _WAN_ARP_AGENT_ON);
        }
    }

    RcuBs_UnLock(uiPhase);

    return eRet;
}



