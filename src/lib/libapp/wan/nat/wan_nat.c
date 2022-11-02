/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-4-7
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/nat_utl.h"
#include "utl/exec_utl.h"
#include "utl/eth_utl.h"
#include "utl/txt_utl.h"
#include "utl/ip_utl.h"
#include "utl/ip_mbuf.h"
#include "utl/sif_utl.h"
#include "utl/lstr_utl.h"
#include "utl/mutex_utl.h"
#include "utl/map_utl.h"
#include "utl/ipfwd_service.h"
#include "utl/json_utl.h"
#include "comp/comp_pcap.h"
#include "comp/comp_if.h"
#include "comp/comp_kfapp.h"

#include "../h/wan_ipfwd.h"
#include "../h/wan_pcap.h"
#include "../h/wan_ip_addr.h"
#include "../h/wan_ipfwd_service.h"
#include "../h/wan_ifnet.h"
#include "../h/wan_nat.h"

#define WAN_NAT_MS_IN_TICK 5000

typedef struct
{
    DLL_NODE_S stLinkNode;
    RCU_NODE_S stRuc;

    IF_INDEX ifIndex;
    NAT_HANDLE hNatHandle;
}_WAN_NAT_IF_CTRL_S;

static MAP_HANDLE g_hWanNatAgg = NULL;
static MUTEX_S g_stWanNatMutex;
static MTIMER_S g_stWanNatMTimer;

static IPFWD_SERVICE_RET_E _wan_nat_DownPktInput
(
    IN IP_HEAD_S *pstIpHead,
    IN MBUF_S *pstMbuf,
    IN USER_HANDLE_S *pstUserHandle
)
{
    UINT uiVFID;
    _WAN_NAT_IF_CTRL_S *pstIfData;
    IF_INDEX ifIndex;
    UINT uiPhase;
    BS_STATUS eRet = BS_ERR;

    ifIndex = MBUF_GET_SEND_IF_INDEX(pstMbuf);

    uiPhase = RcuEngine_Lock();

    pstIfData = MAP_Get(g_hWanNatAgg, &ifIndex, sizeof(IF_INDEX));

    if (NULL != pstIfData)
    {
        uiVFID = MBUF_GET_INVPNID(pstMbuf);
        eRet = NAT_PacketTranslateByMbuf(pstIfData->hNatHandle, pstMbuf, FALSE, &uiVFID);
    }

    RcuEngine_UnLock(uiPhase);

    if (NULL == pstIfData)
    {
        return IPFWD_SERVICE_RET_CONTINUE;
    }

    if (BS_OK != eRet)
    {
        MBUF_Free(pstMbuf);
        return IPFWD_SERVICE_RET_TAKE_OVER;
    }

    WAN_IpFwd_Send(pstMbuf);

    return IPFWD_SERVICE_RET_TAKE_OVER;
}

static IPFWD_SERVICE_RET_E _wan_nat_UpPktInput
(
    IN IP_HEAD_S *pstIpHead,
    IN MBUF_S *pstMbuf,
    IN USER_HANDLE_S *pstUserHandle
)
{
    UINT uiVFID;
    IP_HEAD_S *pstHead;
    _WAN_NAT_IF_CTRL_S *pstIfData;
    IF_INDEX ifIndex;
    UINT uiPhase;
    BS_STATUS eRet = BS_ERR;

    ifIndex = MBUF_GET_RECV_IF_INDEX(pstMbuf);

    uiPhase = RcuEngine_Lock();

    pstIfData = MAP_Get(g_hWanNatAgg, &ifIndex, sizeof(IF_INDEX));

    if (NULL != pstIfData)
    {
        uiVFID = MBUF_GET_INVPNID(pstMbuf);
        eRet = NAT_PacketTranslateByMbuf(pstIfData->hNatHandle, pstMbuf, TRUE, &uiVFID);
    }

    RcuEngine_UnLock(uiPhase);

    if (BS_OK != eRet)
    {
        return IPFWD_SERVICE_RET_CONTINUE;
    }

    pstHead = IP_GetIPHeaderByMbuf(pstMbuf, NET_PKT_TYPE_IP);
    if (NULL == pstHead)
    {
        MBUF_Free(pstMbuf);
        return IPFWD_SERVICE_RET_TAKE_OVER;
    }

    MBUF_SET_OUTVPNID(pstMbuf, uiVFID);
    MBUF_SET_NEXT_HOP(pstMbuf, pstHead->unDstIp.uiIp);
    
    WAN_IpFwd_Send(pstMbuf);

    return IPFWD_SERVICE_RET_TAKE_OVER;
}

static BS_WALK_RET_E _wan_nat_TimeOutEach(IN MAP_ELE_S *pstEle, IN VOID *pUserHandle)
{
    _WAN_NAT_IF_CTRL_S *pstIfData = pstEle->pData;

    NAT_TimerStep(pstIfData->hNatHandle);

    return BS_WALK_CONTINUE;
}

static VOID _wan_nat_TimeOut(IN HANDLE hTimerHandle, IN USER_HANDLE_S *pstUserHandle)
{
    UINT uiPhase;

    uiPhase = RcuEngine_Lock();

    MAP_Walk(g_hWanNatAgg, _wan_nat_TimeOutEach, NULL);

    RcuEngine_UnLock(uiPhase);
}

static VOID _wan_nat_IPAddrEventNotify
(
    IN UINT uiEvent,
    IN IF_INDEX ifIndex,
    IN WAN_IP_ADDR_INFO_S *pstOld,
    IN WAN_IP_ADDR_INFO_S *pstNew,
    IN USER_HANDLE_S *pstUserHandle
)
{
    WAN_IP_ADDR_INFO_S stAddr;
    UINT auiPubIp[ NAT_MAX_PUB_IP_NUM ];
    _WAN_NAT_IF_CTRL_S *pstIfData;
    UINT iPhase;

    iPhase = RcuEngine_Lock();

    pstIfData = MAP_Get(g_hWanNatAgg, &ifIndex, sizeof(IF_INDEX));
    if (NULL != pstIfData)
    {
        Mem_Zero(auiPubIp, sizeof(auiPubIp));

        stAddr.uiIP = 0;
        WAN_IPAddr_GetFirstIp(ifIndex, &stAddr);
        auiPubIp[0] = stAddr.uiIP;

        NAT_SetPubIp(pstIfData->hNatHandle, auiPubIp);
    }

    RcuEngine_UnLock(iPhase);

    return;
}

static BS_STATUS _wan_nat_SetOutBound(IN IF_INDEX ifIndex)
{
    _WAN_NAT_IF_CTRL_S *pstIfData;
    WAN_IP_ADDR_INFO_S stAddr;
    UINT auiPubIp[ NAT_MAX_PUB_IP_NUM ] = {0};

    if (NULL != MAP_Get(g_hWanNatAgg, &ifIndex, sizeof(IF_INDEX)))
    {
        return BS_OK;
    }
    
    pstIfData = MEM_ZMalloc(sizeof(_WAN_NAT_IF_CTRL_S));
    if (NULL == pstIfData)
    {
        return BS_NO_MEMORY;
    }

    pstIfData->hNatHandle = NAT_Create(40000, 65535, WAN_NAT_MS_IN_TICK, TRUE);
    if (NULL == pstIfData->hNatHandle)
    {
        MEM_Free(pstIfData);
        return (BS_NO_MEMORY);
    }

    pstIfData->ifIndex = ifIndex;

    stAddr.uiIP = 0;
    WAN_IPAddr_GetFirstIp(ifIndex, &stAddr);
    auiPubIp[0] = stAddr.uiIP;

    NAT_SetPubIp(pstIfData->hNatHandle, auiPubIp);

    MAP_Add(g_hWanNatAgg, &pstIfData->ifIndex, sizeof(IF_INDEX), pstIfData, 0);
    
    return BS_OK;
}

static VOID _wan_nat_RcuFree(IN VOID *pstRcuNode)
{
    _WAN_NAT_IF_CTRL_S *pstIfData = container_of(pstRcuNode, _WAN_NAT_IF_CTRL_S, stRuc);

    NAT_Destory(pstIfData->hNatHandle);
    MEM_Free(pstIfData);
}

static BS_STATUS _wan_nat_DelOutBound(IN IF_INDEX ifIndex)
{
    _WAN_NAT_IF_CTRL_S *pstIfData;

    pstIfData = MAP_Del(g_hWanNatAgg, &ifIndex, sizeof(IF_INDEX));

    if (NULL != pstIfData)
    {
        RcuEngine_Call(&pstIfData->stRuc, _wan_nat_RcuFree);
    }
    
    return BS_OK;
}

VOID _wan_nat_Save(IN IF_INDEX ifIndex, IN HANDLE hFile)
{
    _WAN_NAT_IF_CTRL_S *pstIfData;
    UINT uiPhase;

    uiPhase = RcuEngine_Lock();
    pstIfData = MAP_Get(g_hWanNatAgg, &ifIndex, sizeof(IF_INDEX));
    RcuEngine_UnLock(uiPhase);

    if (NULL == pstIfData)
    {
        return;
    }

    CMD_EXP_OutputCmd(hFile, "nat outbound");
}

static VOID _wan_nat_IfDelEvent(IN UINT uiIfIndex)
{
    WAN_NAT_DelOutBound(uiIfIndex);
}

static BS_STATUS _wan_nat_IfEvent(IN UINT uiIfIndex, IN UINT uiEvent, IN USER_HANDLE_S *pstUserHandle)
{
    switch (uiEvent)
    {
        case IF_EVENT_DELETE:
        {
            _wan_nat_IfDelEvent(uiIfIndex);
            break;
        }

        default:
        {
            break;
        }
    }

    return BS_OK;
}

static BS_WALK_RET_E _wan_nat_SetDbgFlag(IN MAP_ELE_S *pstEle, IN VOID *pUserHandle)
{
    _WAN_NAT_IF_CTRL_S *pstIfData = pstEle->pData;
    UINT uiFlag = HANDLE_UINT(pUserHandle);
    
    NAT_SetDbgFlag(pstIfData->hNatHandle, uiFlag);

    return BS_WALK_CONTINUE;
}

static BS_WALK_RET_E _wan_nat_ClrDbgFlag(IN MAP_ELE_S *pstEle, IN VOID *pUserHandle)
{
    _WAN_NAT_IF_CTRL_S *pstIfData = pstEle->pData;
    UINT uiFlag = HANDLE_UINT(pUserHandle);
    
    NAT_ClrDbgFlag(pstIfData->hNatHandle, uiFlag);

    return BS_WALK_CONTINUE;
}

static BS_STATUS _wan_nat_kf_Add(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    CHAR *pcInterface;
    IF_INDEX ifIndex;

    pcInterface = MIME_GetKeyValue(hMime, "Interface");
    if (NULL == pcInterface)
    {
        JSON_SetFailed(pstParam->pstJson, "Bad param");
        return BS_OK;
    }

    ifIndex = IFNET_GetIfIndex(pcInterface);
    if (ifIndex == IF_INVALID_INDEX)
    {
        JSON_SetFailed(pstParam->pstJson, "Interface not exist");
        return BS_OK;
    }

    WAN_NAT_SetOutBound(ifIndex);

    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}

static BS_STATUS _wan_nat_kf_Del(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    CHAR *pcNames;
    LSTR_S stName;
    IF_INDEX ifIndex;
    CHAR szName[IF_MAX_NAME_LEN + 1];

    pcNames = MIME_GetKeyValue(hMime, "Delete");
    if (NULL == pcNames)
    {
        JSON_SetSuccess(pstParam->pstJson);
        return BS_OK;
    }

    LSTR_SCAN_ELEMENT_BEGIN(pcNames, strlen(pcNames), ',', &stName)
    {
        if ((stName.uiLen != 0) && (stName.uiLen < sizeof(szName)))
        {
            TXT_Strlcpy(szName, stName.pcData, stName.uiLen + 1);
            ifIndex = IFNET_GetIfIndex(szName);
            if (ifIndex != IF_INVALID_INDEX)
            {
                WAN_NAT_DelOutBound(ifIndex);
            }
        }
    }LSTR_SCAN_ELEMENT_END();

    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}


static BS_STATUS _wan_nat_kf_List(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    UINT uiPhase;
    cJSON *pstArray;
    cJSON *pstResJson;
    MAP_ELE_S *pstEle = NULL;
    _WAN_NAT_IF_CTRL_S *pstNatIfCtrl;
    CHAR szIfName[IF_MAX_NAME_LEN + 1];
    
    pstArray = cJSON_CreateArray();
    if (NULL == pstArray)
    {
        JSON_SetFailed(pstParam->pstJson, "Not enough memory");
        return BS_OK;
    }

    uiPhase = RcuEngine_Lock();

    while (NULL != (pstEle = MAP_GetNext(g_hWanNatAgg, pstEle)))
    {
        pstNatIfCtrl = pstEle->pData;

        pstResJson = cJSON_CreateObject();
        if (NULL == pstResJson)
        {
            continue;
        }

        if (BS_OK != IFNET_Ioctl(pstNatIfCtrl->ifIndex, IFNET_CMD_GET_IFNAME, szIfName))
        {
            continue;
        }
        
        cJSON_AddStringToObject(pstResJson, "Name", szIfName);
        cJSON_AddStringToObject(pstResJson, "Interface", szIfName);

        cJSON_AddItemToArray(pstArray, pstResJson);
    }

    RcuEngine_UnLock(uiPhase);

    cJSON_AddItemToObject(pstParam->pstJson, "data", pstArray);

    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}

BS_STATUS WAN_NAT_SetOutBound(IN IF_INDEX ifIndex)
{
    BS_STATUS eRet;

    MUTEX_P(&g_stWanNatMutex);
    eRet = _wan_nat_SetOutBound(ifIndex);
    MUTEX_V(&g_stWanNatMutex);

    return eRet;
}

VOID WAN_NAT_DelOutBound(IN IF_INDEX ifIndex)
{
    MUTEX_P(&g_stWanNatMutex);
    _wan_nat_DelOutBound(ifIndex);
    MUTEX_V(&g_stWanNatMutex);
}

BS_STATUS WAN_NAT_Init()
{
    int ret;

    IFNET_RegEvent(_wan_nat_IfEvent, NULL);
    IFNET_RegSave(_wan_nat_Save);

    MUTEX_Init(&g_stWanNatMutex);

    g_hWanNatAgg = MAP_HashCreate(0);
    if (NULL == g_hWanNatAgg)
    {
        return BS_NO_MEMORY;
    }

    ret = MTimer_Add(&g_stWanNatMTimer, WAN_NAT_MS_IN_TICK,
            TIMER_FLAG_CYCLE, _wan_nat_TimeOut, NULL);
    if (ret < 0)
    {
        return BS_CAN_NOT_OPEN;
    }

    WAN_IPAddr_RegListener(_wan_nat_IPAddrEventNotify, NULL);
    
    if (BS_OK != WAN_IpFwdService_Reg(IPFWD_SERVICE_BEFORE_FORWARD, 0, "nat", _wan_nat_DownPktInput, NULL))
    {
        return BS_ERR;
    }

    if (BS_OK != WAN_IpFwdService_Reg(IPFWD_SERVICE_BEFORE_DELIVER_UP, 0, "nat", _wan_nat_UpPktInput, NULL))
    {
        return BS_ERR;
    }

    return BS_OK;
}

BS_STATUS WAN_NAT_KfInit()
{
    KFAPP_RegFunc("wan.nat.Add", _wan_nat_kf_Add, NULL);
    KFAPP_RegFunc("wan.nat.Delete", _wan_nat_kf_Del, NULL);
    KFAPP_RegFunc("wan.nat.List", _wan_nat_kf_List, NULL);

	return BS_OK;
}

/*
 接口视图下下:
  [no] nat outbound
*/
PLUG_API BS_STATUS WAN_NAT_SetOutBoundCmd
(
    IN UINT ulArgc,
    IN CHAR **argv,
    IN VOID *pEnv
)
{
    IF_INDEX ifIndex;

    ifIndex = WAN_IF_GetIfIndexByEnv(pEnv);
    if (0 == ifIndex)
    {
        EXEC_OutString("Can't get ifindex.\r\n");
        return BS_ERR;
    }

    if (strcmp("no", argv[0]) == 0)
    {
        WAN_NAT_DelOutBound(ifIndex);
    }
    else
    {
        WAN_NAT_SetOutBound(ifIndex);
    }

    return BS_OK;
}

PLUG_API BS_STATUS WAN_NAT_DebugPacket(IN UINT ulArgc, IN CHAR **argv)
{
    UINT uiPhase;

    uiPhase = RcuEngine_Lock();
    MAP_Walk(g_hWanNatAgg, _wan_nat_SetDbgFlag, UINT_HANDLE(NAT_DBG_PACKET));
    RcuEngine_UnLock(uiPhase);

	return BS_OK;
}

PLUG_API BS_STATUS WAN_NAT_NoDebugPacket(IN UINT ulArgc, IN CHAR **argv)
{
    UINT uiPhase;

    uiPhase = RcuEngine_Lock();
    MAP_Walk(g_hWanNatAgg, _wan_nat_ClrDbgFlag, UINT_HANDLE(NAT_DBG_PACKET));
    RcuEngine_UnLock(uiPhase);

    return BS_OK;
}


