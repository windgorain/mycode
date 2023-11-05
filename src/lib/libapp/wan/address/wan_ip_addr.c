/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-4-2
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/sprintf_utl.h"
#include "utl/fib_utl.h"
#include "utl/ob_chain.h"
#include "utl/bit_opt.h"
#include "utl/ip_utl.h"
#include "utl/ip_string.h"
#include "utl/json_utl.h"
#include "comp/comp_if.h"
#include "comp/comp_kfapp.h"
#include "comp/comp_wan.h"

#include "../h/wan_ifnet.h"
#include "../h/wan_fib.h"
#include "../h/wan_dhcp.h"
#include "../h/wan_inloop.h"
#include "../h/wan_ip_addr.h"

typedef struct
{
    WAN_IP_ADDR_S stIpAddrs;
}_IPADDR_IF_CTRL_S;

typedef struct
{
    DLL_NODE_S stLinkNode;
    UINT uiVrf;
    WAN_IP_ADDR_INFO_S stIpAddrInfo;
}_IPADDR_LIST_NODE_S;

static OB_CHAIN_S g_stWanIpAddrObserverList = OB_CHAIN_HEAD_INIT_VALUE(&g_stWanIpAddrObserverList);
static UINT g_uiWanIpAddrIfUserDataIndex = 0;
static DLL_HEAD_S g_stWanIpAddrIpList = DLL_HEAD_INIT_VALUE(&g_stWanIpAddrIpList);

static VOID _wan_ipaddr_EventNotify
(
    IN IF_INDEX ifIndex,
    IN WAN_IP_ADDR_INFO_S *pstOld,
    IN WAN_IP_ADDR_INFO_S *pstNew
)
{
    UINT uiEvent;
    BOOL_T bHaveOld = FALSE;
    BOOL_T bHaveNew = FALSE;

    if ((NULL != pstOld) && (pstOld->uiIP != 0)) {
        bHaveOld = TRUE;
    }

    if ((NULL != pstNew) && (pstNew->uiIP != 0)) {
        bHaveNew = TRUE;
    }

    if (bHaveOld == FALSE) {
        if (bHaveNew == FALSE) {
            return;
        } else {
            uiEvent = WAN_IPADDR_EVENT_ADD_ADDR;
        }
    } else {
        if (bHaveNew == FALSE) {
            uiEvent = WAN_IPADDR_EVENT_DEL_ADDR;
        } else {
            uiEvent = WAN_IPADDR_EVENT_MODIFY_ADDR;
        }
    }

    OB_CHAIN_NOTIFY(&g_stWanIpAddrObserverList, PF_WAN_IPAddr_EventNotify,
            ifIndex, (ULONG)uiEvent, pstOld, pstNew);
}

static VOID wan_ipaddr_IfCreateEvent(IN UINT uiIfIndex)
{
    _IPADDR_IF_CTRL_S *pstIpAddrCtrl;
    pstIpAddrCtrl = MEM_ZMalloc(sizeof(_IPADDR_IF_CTRL_S));
    if (NULL == pstIpAddrCtrl)
    {
        return;
    }
    IFNET_SetUserData(uiIfIndex, g_uiWanIpAddrIfUserDataIndex, pstIpAddrCtrl);
}

static VOID wan_ipaddr_IfDelEvent(IN UINT uiIfIndex)
{
    _IPADDR_IF_CTRL_S *pstIpAddrCtrl;
    UINT i;
    
    if (BS_OK != IFNET_GetUserData(uiIfIndex, g_uiWanIpAddrIfUserDataIndex, (VOID*)&pstIpAddrCtrl))
    {
        return;
    }

    if (pstIpAddrCtrl != NULL)
    {
        for (i=0; i<WAN_IP_ADDR_MAX_IF_IP_NUM; i++)
        {
            if (pstIpAddrCtrl->stIpAddrs.astIP[i].uiIP != 0)
            {
                _wan_ipaddr_EventNotify(uiIfIndex, &pstIpAddrCtrl->stIpAddrs.astIP[i], NULL);
            }
        }
        
        MEM_Free(pstIpAddrCtrl);
    }
}

static BS_STATUS wan_ipaddr_IfEvent(IN UINT uiIfIndex, IN UINT uiEvent, IN USER_HANDLE_S *pstUserHandle)
{
    switch (uiEvent)
    {
        case IF_EVENT_CREATE:
        {
            wan_ipaddr_IfCreateEvent(uiIfIndex);
            break;
        }

        case IF_EVENT_DELETE:
        {
            wan_ipaddr_IfDelEvent(uiIfIndex);
            break;
        }

        default:
        {
            break;
        }
    }

    return BS_OK;
}

static VOID wan_ipaddr_DelRoute(IN UINT uiVrf, IN WAN_IP_ADDR_INFO_S *pstAddrInfo)
{
    FIB_NODE_S stFib;

    memset(&stFib, 0, sizeof(stFib));

    
    stFib.stFibKey.uiDstOrStartIp = pstAddrInfo->uiIP;
    stFib.stFibKey.uiMaskOrEndIp = pstAddrInfo->uiMask;
    stFib.uiNextHop = 0;
    WanFib_Del(uiVrf, &stFib);

    
    stFib.stFibKey.uiDstOrStartIp = pstAddrInfo->uiIP;
    stFib.stFibKey.uiMaskOrEndIp = 0xffffffff;
    stFib.uiNextHop = 0;
    WanFib_Del(uiVrf, &stFib);
}

static BS_STATUS _wan_ipaddr_kf_Get(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    CHAR *pcName;
    IF_INDEX ifIndex;
    UINT i;
    _IPADDR_IF_CTRL_S *pstIpAddrCtrl;
    cJSON *pstArray;
    cJSON *pstNode;
    CHAR szInfo[32];

    pcName = MIME_GetKeyValue(hMime, "Interface");
    if ((NULL == pcName) || (pcName[0] == '\0'))
    {
        JSON_SetFailed(pstParam->pstJson, "Empty interface");
        return BS_OK;
    }

    cJSON_AddStringToObject(pstParam->pstJson, "Interface", pcName);

    ifIndex = IFNET_GetIfIndex(pcName);
    if (IF_INVALID_INDEX == ifIndex)
    {
        JSON_SetFailed(pstParam->pstJson, "Not exist");
        return BS_OK;
    }

    if ((BS_OK != IFNET_GetUserData(ifIndex, g_uiWanIpAddrIfUserDataIndex, (VOID*)&pstIpAddrCtrl))
        || (pstIpAddrCtrl == NULL))
    {
        JSON_SetFailed(pstParam->pstJson, "Not exist");
        return BS_OK;
    }

    pstArray = cJSON_CreateArray();
    if (NULL == pstArray)
    {
        JSON_SetFailed(pstParam->pstJson, "Not enough memory");
        return BS_OK;
    }

    for (i=0; i<WAN_IP_ADDR_MAX_IF_IP_NUM; i++)
    {
        if (pstIpAddrCtrl->stIpAddrs.astIP[i].uiIP != 0)
        {
            pstNode = cJSON_CreateObject();
            if (NULL == pstNode)
            {
                continue;
            }

            BS_Sprintf(szInfo, "%pI4/%pI4",
                &pstIpAddrCtrl->stIpAddrs.astIP[i].uiIP, &pstIpAddrCtrl->stIpAddrs.astIP[i].uiMask);
            cJSON_AddStringToObject(pstNode, "IP", szInfo);
            cJSON_AddItemToArray(pstArray, pstNode);
        }
    }

    cJSON_AddItemToObject(pstParam->pstJson, "Address", pstArray);

    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}

static BOOL_T _wan_ipaddr_isConfiged(IN _IPADDR_IF_CTRL_S *pstIpAddrCtrl, IN UINT uiIP, IN UINT uiMask)
{
    UINT i;

    for (i=0; i<WAN_IP_ADDR_MAX_IF_IP_NUM; i++)
    {
        if ((pstIpAddrCtrl->stIpAddrs.astIP[i].uiIP == uiIP)
            && (pstIpAddrCtrl->stIpAddrs.astIP[i].uiMask == uiMask))
        {
            return TRUE;
        }
    }

    return FALSE;
}

static BS_STATUS _wan_ipaddr_kf_ModifyIpMask(IN IF_INDEX ifIndex, IN IP_MASK_S *pstIpMask, IN UINT uiNum)
{
    _IPADDR_IF_CTRL_S *pstIpAddrCtrl;
    UINT i;
    UINT uiIP;
    UINT uiMask;
    WAN_IP_ADDR_INFO_S stInfo;
    BS_STATUS eRet = BS_OK;

    if ((BS_OK != IFNET_GetUserData(ifIndex, g_uiWanIpAddrIfUserDataIndex, (VOID*)&pstIpAddrCtrl))
        || (pstIpAddrCtrl == NULL))
    {
        return BS_ERR;
    }

    for (i=0; i<WAN_IP_ADDR_MAX_IF_IP_NUM; i++)
    {
        if (pstIpAddrCtrl->stIpAddrs.astIP[i].uiIP == 0)
        {
            continue;
        }

        if (IPUtl_IsExistInIpArry(pstIpMask, uiNum,
            ntohl(pstIpAddrCtrl->stIpAddrs.astIP[i].uiIP), ntohl(pstIpAddrCtrl->stIpAddrs.astIP[i].uiMask)))
        {
            continue;
        }

        WAN_IPAddr_DelIp(ifIndex, pstIpAddrCtrl->stIpAddrs.astIP[i].uiIP);
    }

    for (i=0; i<uiNum; i++)
    {
        uiIP = htonl(pstIpMask[i].uiIP);
        uiMask = htonl(pstIpMask[i].uiMask);
        if (_wan_ipaddr_isConfiged(pstIpAddrCtrl, uiIP, uiMask))
        {
            continue;
        }

        stInfo.uiIfIndex = ifIndex;
        stInfo.uiIP = uiIP;
        stInfo.uiMask = uiMask;

        eRet |= WanIPAddr_AddIp(&stInfo);
    }

    return eRet;
}

static BS_STATUS _wan_ipaddr_kf_Modify(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    CHAR *pcName;
    CHAR *pcPropertyValue;
    IF_INDEX ifIndex;
    IP_MASK_S stIpMasks[WAN_IP_ADDR_MAX_IF_IP_NUM];
    UINT uiNum;
    BS_STATUS eRet = BS_OK;

    pcName = MIME_GetKeyValue(hMime, "Interface");
    if ((NULL == pcName) || (pcName[0] == '\0'))
    {
        JSON_SetFailed(pstParam->pstJson, "Empty interface");
        return BS_OK;
    }

    ifIndex = IFNET_GetIfIndex(pcName);
    if (IF_INVALID_INDEX == ifIndex)
    {
        JSON_SetFailed(pstParam->pstJson, "Not exist");
        return BS_OK;
    }

    pcPropertyValue = MIME_GetKeyValue(hMime, "IP");
    if (NULL == pcPropertyValue)
    {
        pcPropertyValue = "";
    }

    uiNum = IPString_ParseIpMaskList(pcPropertyValue, ',', WAN_IP_ADDR_MAX_IF_IP_NUM, &stIpMasks[0]);
    eRet = _wan_ipaddr_kf_ModifyIpMask(ifIndex, stIpMasks, uiNum);

    if (eRet != BS_OK)
    {
        JSON_SetFailed(pstParam->pstJson, "IPAddress error");
    }
    else
    {
        JSON_SetSuccess(pstParam->pstJson);
    }

    return BS_OK;
}

BS_STATUS WAN_IPAddr_KfInit()
{
    KFAPP_RegFunc("IPAddress.Modify", _wan_ipaddr_kf_Modify, NULL);
    KFAPP_RegFunc("IPAddress.Get", _wan_ipaddr_kf_Get, NULL);

    return BS_OK;
}

BS_STATUS WAN_IPAddr_Init()
{
    g_uiWanIpAddrIfUserDataIndex = IFNET_AllocUserDataIndex();

    return IFNET_RegEvent(wan_ipaddr_IfEvent, NULL);
}


BS_STATUS WAN_IPAddr_MatchNetCover(IN UINT uiIfIndex, IN UINT uiIpAddr, OUT WAN_IP_ADDR_INFO_S *pstAddr)
{
    _IPADDR_IF_CTRL_S *pstIpAddrCtrl;
    WAN_IP_ADDR_INFO_S *pstFound = NULL;
    WAN_IP_ADDR_INFO_S *pstTmp;
    UINT i;

    if ((BS_OK != IFNET_GetUserData(uiIfIndex, g_uiWanIpAddrIfUserDataIndex, (VOID*)&pstIpAddrCtrl))
        || (pstIpAddrCtrl == NULL))
    {
        return BS_NOT_FOUND;
    }

    for (i=0; i<WAN_IP_ADDR_MAX_IF_IP_NUM; i++)
    {
        pstTmp = &pstIpAddrCtrl->stIpAddrs.astIP[i];
        
        if ((pstTmp->uiIP != 0) && ((uiIpAddr & pstTmp->uiMask) == (pstTmp->uiIP & pstTmp->uiMask)))
        {
            if ((pstFound == NULL) || (pstTmp->uiMask > pstFound->uiMask))
            {
                pstFound = &pstIpAddrCtrl->stIpAddrs.astIP[i];
            }
        }
    }

    if (NULL != pstFound)
    {
        *pstAddr = *pstFound;
        return BS_OK;
    }

    return BS_NOT_FOUND;
}


static BS_STATUS wan_ipaddr_MatchBestNetNormal
(
    IN UINT uiIfIndex,
    IN UINT uiIpAddr,
    OUT WAN_IP_ADDR_INFO_S *pstAddr
)
{
    _IPADDR_IF_CTRL_S *pstIpAddrCtrl;
    WAN_IP_ADDR_INFO_S *pstFound = NULL;
    UINT uiPrefix = 0;
    UINT uiPrefixTmp;
    UINT i;
    UINT uiIpAddrHost = ntohl(uiIpAddr);

    if ((BS_OK != IFNET_GetUserData(uiIfIndex, g_uiWanIpAddrIfUserDataIndex, (VOID*)&pstIpAddrCtrl))
        || (pstIpAddrCtrl == NULL))
    {
        return BS_NOT_FOUND;
    }

    for (i=0; i<WAN_IP_ADDR_MAX_IF_IP_NUM; i++)
    {
        if (pstIpAddrCtrl->stIpAddrs.astIP[i].uiIP == 0)
        {
            continue;
        }

        uiPrefixTmp = IP_GetCommonPrefix(uiIpAddrHost, ntohl(pstIpAddrCtrl->stIpAddrs.astIP[i].uiIP));
        if (uiPrefixTmp > uiPrefix)
        {
            uiPrefix = uiPrefixTmp;
            pstFound = &pstIpAddrCtrl->stIpAddrs.astIP[i];
        }
    }

    if (NULL != pstFound)
    {
        *pstAddr = *pstFound;
        return BS_OK;
    }

    return BS_NOT_FOUND;
}



BS_STATUS WAN_IPAddr_MatchBestNet(IN UINT uiIfIndex, IN UINT uiIpAddr, OUT WAN_IP_ADDR_INFO_S *pstAddr)
{
    if (BS_OK == WAN_IPAddr_MatchNetCover(uiIfIndex, uiIpAddr, pstAddr))
    {
        return BS_OK;
    }

    return wan_ipaddr_MatchBestNetNormal(uiIfIndex, uiIpAddr, pstAddr);
}


BS_STATUS WAN_IPAddr_FindVrfIp
(
    IN UINT uiVrf,
    IN UINT uiIp, 
    OUT WAN_IP_ADDR_INFO_S *pstAddrInfoFound
)
{
    _IPADDR_LIST_NODE_S *pstNode;

    DLL_SCAN(&g_stWanIpAddrIpList, pstNode)
    {
        if ((pstNode->uiVrf == uiVrf)
            && (pstNode->stIpAddrInfo.uiIP == uiIp))
        {
            break;
        }
    }

    if (pstNode == NULL)
    {
        return BS_NOT_FOUND;
    }

    if (NULL != pstAddrInfoFound)
    {
        *pstAddrInfoFound = pstNode->stIpAddrInfo;
    }

    return BS_OK;
}


BS_STATUS WAN_IPAddr_FindSameNet
(
    IN UINT uiVrf,
    IN UINT uiIp, 
    IN UINT uiMask, 
    OUT WAN_IP_ADDR_INFO_S *pstAddrInfoFound
)
{
    _IPADDR_LIST_NODE_S *pstNode;

    uiIp = uiIp & uiMask;

    DLL_SCAN(&g_stWanIpAddrIpList, pstNode)
    {
        if ((pstNode->uiVrf == uiVrf)
            && (pstNode->stIpAddrInfo.uiMask == uiMask)
            && ((pstNode->stIpAddrInfo.uiIP & uiMask) == uiIp))
        {
            break;
        }
    }

    if (pstNode == NULL)
    {
        return BS_NOT_FOUND;
    }

    if (NULL != pstAddrInfoFound)
    {
        *pstAddrInfoFound = pstNode->stIpAddrInfo;
    }

    return BS_OK;
}


BS_STATUS WAN_IPAddr_FindConflictIP
(
    IN UINT uiVrf,
    IN UINT uiIp, 
    IN UINT uiMask, 
    OUT WAN_IP_ADDR_INFO_S *pstAddrInfoFound  
)
{
    if (BS_OK == WAN_IPAddr_FindVrfIp(uiVrf, uiIp, pstAddrInfoFound))
    {
        return BS_OK;
    }

    return WAN_IPAddr_FindSameNet(uiVrf, uiIp, uiMask, pstAddrInfoFound);
}

BOOL_T WAN_IPAddr_IsInterfaceIp(IN UINT uiIfIndex, IN UINT uiIP)
{
    _IPADDR_IF_CTRL_S *pstIpAddrCtrl;
    UINT i;

    if (uiIP == 0)
    {
        return FALSE;
    }

    if ((BS_OK != IFNET_GetUserData(uiIfIndex, g_uiWanIpAddrIfUserDataIndex, (VOID*)&pstIpAddrCtrl))
        || (pstIpAddrCtrl == NULL))
    {
        return FALSE;
    }

    for (i=0; i<WAN_IP_ADDR_MAX_IF_IP_NUM; i++)
    {
        if (uiIP == pstIpAddrCtrl->stIpAddrs.astIP[i].uiIP)
        {
            return TRUE;
        }
    }

    return FALSE;
}

static BS_STATUS wan_ipaddr_AddSubIP(IN _IPADDR_IF_CTRL_S *pstIpAddrCtrl, IN WAN_IP_ADDR_INFO_S *pstAddrInfo)
{
    UINT i;
    
    for (i=0; i<WAN_IP_ADDR_MAX_IF_IP_NUM; i++)
    {
        if (pstIpAddrCtrl->stIpAddrs.astIP[i].uiIP == 0)
        {
            pstIpAddrCtrl->stIpAddrs.astIP[i] = *pstAddrInfo;
            return BS_OK;
        }
    }

    return BS_FULL;
}

static VOID wan_ipaddr_DelIpFromList(IN UINT uiVrf, IN WAN_IP_ADDR_INFO_S *pstIpInfo)
{
    _IPADDR_LIST_NODE_S *pstNode;

    DLL_SCAN(&g_stWanIpAddrIpList, pstNode)
    {
        if ((pstNode->uiVrf == uiVrf) && (pstNode->stIpAddrInfo.uiIP == pstIpInfo->uiIP))
        {
            break;
        }
    }

    if (pstNode != NULL)
    {
        DLL_DEL(&g_stWanIpAddrIpList, pstNode);
        MEM_Free(pstNode);
    }

    return;
}

PLUG_API BS_STATUS WanIPAddr_SetMode(IN IF_INDEX ifIndex, IN WAN_IP_ADDR_MODE_E enMode)
{
    _IPADDR_IF_CTRL_S *pstIpAddrCtrl;

    if (BS_OK != IFNET_GetUserData(ifIndex, g_uiWanIpAddrIfUserDataIndex, (VOID*)&pstIpAddrCtrl))
    {
        return BS_ERR;
    }

    if (pstIpAddrCtrl->stIpAddrs.enIpMode == enMode)
    {
        return BS_OK;
    }

    WAN_IPAddr_DelInterfaceAllIp(ifIndex);

    return BS_OK;
}

WAN_IP_ADDR_MODE_E WAN_IPAddr_GetMode(IN IF_INDEX ifIndex)
{
    _IPADDR_IF_CTRL_S *pstIpAddrCtrl;

    if (BS_OK != IFNET_GetUserData(ifIndex, g_uiWanIpAddrIfUserDataIndex, (VOID*)&pstIpAddrCtrl))
    {
        return WAN_IP_ADDR_MODE_MAX;
    }

    return pstIpAddrCtrl->stIpAddrs.enIpMode;
}

PLUG_API BS_STATUS WanIPAddr_AddIp(IN WAN_IP_ADDR_INFO_S *pstAddrInfo)
{
    FIB_NODE_S stFib;
    WAN_IP_ADDR_INFO_S stOld;
    WAN_IP_ADDR_INFO_S *pstOld = NULL;
    _IPADDR_IF_CTRL_S *pstIpAddrCtrl;
    _IPADDR_LIST_NODE_S *pstNode;
    WAN_IP_ADDR_INFO_S stConflict;
    UINT uiVrf = 0;

    if (BS_OK != IFNET_GetUserData(pstAddrInfo->uiIfIndex, g_uiWanIpAddrIfUserDataIndex, (VOID*)&pstIpAddrCtrl))
    {
        return BS_ERR;
    }

    if (pstIpAddrCtrl == NULL)
    {
        return BS_ERR;
    }

    if (BS_OK == WAN_IPAddr_FindConflictIP(uiVrf, pstAddrInfo->uiIP, pstAddrInfo->uiMask, &stConflict))
    {
        return BS_CONFLICT;
    }

    pstNode = MEM_ZMalloc(sizeof(_IPADDR_LIST_NODE_S));
    if (NULL == pstNode)
    {
        return BS_NO_MEMORY;
    }

    pstNode->stIpAddrInfo = *pstAddrInfo;
    pstNode->uiVrf = uiVrf;

    stOld.uiIP = 0;

    if (BS_OK != wan_ipaddr_AddSubIP(pstIpAddrCtrl, pstAddrInfo))
    {
        MEM_Free(pstNode);
        return BS_FULL;
    }

    DLL_ADD(&g_stWanIpAddrIpList, pstNode);

    if (stOld.uiIP != 0)
    {
        wan_ipaddr_DelRoute(uiVrf, &stOld);
    }

    memset(&stFib, 0, sizeof(stFib));

    
    stFib.stFibKey.uiDstOrStartIp = pstAddrInfo->uiIP;
    stFib.stFibKey.uiMaskOrEndIp = pstAddrInfo->uiMask;
    stFib.uiOutIfIndex = pstAddrInfo->uiIfIndex;
    stFib.uiNextHop = 0;
    stFib.uiFlag = FIB_FLAG_DIRECT;
    WanFib_Add(uiVrf, &stFib);

    
    stFib.stFibKey.uiDstOrStartIp = pstAddrInfo->uiIP;
    stFib.stFibKey.uiMaskOrEndIp = 0xffffffff;
    stFib.uiOutIfIndex = WAN_InLoop_GetIfIndex();
    stFib.uiNextHop = 0;
    stFib.uiFlag = FIB_FLAG_DELIVER_UP;
    WanFib_Add(uiVrf, &stFib);

    _wan_ipaddr_EventNotify(pstAddrInfo->uiIfIndex, pstOld, pstAddrInfo);

    return BS_OK;
}

BS_STATUS WAN_IPAddr_DelIp(IN IF_INDEX ifIndex, IN UINT uiIP)
{
    WAN_IP_ADDR_INFO_S *pstOld = NULL;
    _IPADDR_IF_CTRL_S *pstIpAddrCtrl;
    UINT uiVrf = 0;
    INT i;

    if (uiIP == 0)
    {
        return BS_BAD_PARA;
    }

    if (BS_OK != IFNET_GetUserData(ifIndex, g_uiWanIpAddrIfUserDataIndex, (VOID*)&pstIpAddrCtrl))
    {
        return BS_ERR;
    }

    if (pstIpAddrCtrl == NULL)
    {
        return BS_ERR;
    }

    for (i=0; i<WAN_IP_ADDR_MAX_IF_IP_NUM; i++)
    {
        if (pstIpAddrCtrl->stIpAddrs.astIP[i].uiIP == uiIP)
        {
            pstOld = &pstIpAddrCtrl->stIpAddrs.astIP[i];
            break;
        }
    }

    if (NULL == pstOld)
    {
        return BS_OK;
    }

    wan_ipaddr_DelIpFromList(uiVrf, pstOld);
    wan_ipaddr_DelRoute(uiVrf, pstOld);

    _wan_ipaddr_EventNotify(ifIndex, pstOld, NULL);

    pstIpAddrCtrl->stIpAddrs.astIP[i].uiIP = 0;

    return BS_OK;
}

BS_STATUS WAN_IPAddr_GetInterfaceAllIp(IN IF_INDEX ifIndex, OUT WAN_IP_ADDR_S *pstIpAddrs)
{
    _IPADDR_IF_CTRL_S *pstIpAddrCtrl;

    if ((BS_OK != IFNET_GetUserData(ifIndex, g_uiWanIpAddrIfUserDataIndex, (VOID*)&pstIpAddrCtrl))
        || (NULL == pstIpAddrCtrl))
    {
        return BS_ERR;
    }

    memcpy(pstIpAddrs, &pstIpAddrCtrl->stIpAddrs, sizeof(WAN_IP_ADDR_S));

    return BS_OK;
}

BS_STATUS WAN_IPAddr_DelInterfaceAllIp(IN IF_INDEX ifIndex)
{
    _IPADDR_IF_CTRL_S *pstIpAddrCtrl;
    INT i;

    if ((BS_OK != IFNET_GetUserData(ifIndex, g_uiWanIpAddrIfUserDataIndex, (VOID*)&pstIpAddrCtrl))
        || (NULL == pstIpAddrCtrl))
    {
        return BS_ERR;
    }

    for (i=0; i<WAN_IP_ADDR_MAX_IF_IP_NUM; i++)
    {
        WAN_IPAddr_DelIp(ifIndex, pstIpAddrCtrl->stIpAddrs.astIP[i].uiIP);
    }

    return BS_OK;
}


BS_STATUS WAN_IPAddr_GetFirstIp(IN UINT uiIfIndex, OUT WAN_IP_ADDR_INFO_S *pstAddr)
{
    _IPADDR_IF_CTRL_S *pstIpAddrCtrl;
    UINT i;

    if ((BS_OK != IFNET_GetUserData(uiIfIndex, g_uiWanIpAddrIfUserDataIndex, (VOID*)&pstIpAddrCtrl))
        || (NULL == pstIpAddrCtrl))
    {
        return BS_NO_SUCH;
    }

    for (i=0; i<WAN_IP_ADDR_MAX_IF_IP_NUM; i++)
    {
        if (pstIpAddrCtrl->stIpAddrs.astIP[i].uiIP != 0)
        {
            *pstAddr = pstIpAddrCtrl->stIpAddrs.astIP[i];
            return BS_OK;
        }
    }

    return BS_NOT_FOUND;
}

BS_STATUS WAN_IPAddr_RegListener(IN PF_WAN_IPAddr_EventNotify pfFunc, IN USER_HANDLE_S *pstUserHandle)
{
    return OB_CHAIN_Add(&g_stWanIpAddrObserverList, (void *)pfFunc, pstUserHandle);
}

