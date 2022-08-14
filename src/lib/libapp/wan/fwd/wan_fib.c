/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-3-20
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/sif_utl.h"
#include "utl/socket_utl.h"
#include "utl/exec_utl.h"
#include "utl/fib_utl.h"
#include "utl/mime_utl.h"
#include "utl/range_fib.h"
#include "utl/ipfwd_service.h"
#include "utl/vf_utl.h"
#include "utl/json_utl.h"
#include "utl/txt_utl.h"
#include "utl/lstr_utl.h"
#include "comp/comp_if.h"
#include "comp/comp_kfapp.h"
#include "comp/comp_wan.h"

#include "../h/wan_vrf.h"
#include "../h/wan_ifnet.h"
#include "../h/wan_ipfwd_service.h"
#include "../h/wan_fib.h"
#include "../h/wan_vrf_cmd.h"
#include "../h/wan_ip_addr.h"
#include "../h/wan_ipfwd.h"
#include "../h/wan_blackhole.h"

typedef struct
{
    RCU_NODE_S stRcu;
    RANGE_FIB_HANDLE hRangeFib;
    FIB_HANDLE hFib;
}_WAN_FIB_S;

static BS_STATUS wan_fib_VFEventCreate(IN UINT uiVrfID)
{
    _WAN_FIB_S *pstFib;

    pstFib = MEM_ZMalloc(sizeof(_WAN_FIB_S));
    if (NULL == pstFib)
    {
        return BS_NO_MEMORY;
    }

    pstFib->hRangeFib = RangeFib_Create(TRUE);
    if (NULL == pstFib->hRangeFib)
    {
        MEM_Free(pstFib);
        return (BS_NO_MEMORY);
    }

    pstFib->hFib = FIB_Create(FIB_INSTANCE_FLAG_CREATE_LOCK);
    if (NULL == pstFib->hFib)
    {
        RangeFib_Destory(pstFib->hRangeFib);
        MEM_Free(pstFib);
        return (BS_NO_MEMORY);
    }

    WanVrf_SetData(uiVrfID, WAN_VRF_PROPERTY_INDEX_FIB, pstFib);

    return BS_OK;
}

static VOID _wan_fib_RcuFree(IN VOID *pstRcuNode)
{
    _WAN_FIB_S *pstFib = pstRcuNode;

    RangeFib_Destory(pstFib->hRangeFib);
    FIB_Destory(pstFib->hFib);
    MEM_Free(pstFib);
}

static VOID wan_fib_VFEventDestory(IN UINT uiVrfID)
{
    _WAN_FIB_S *pstFib;

    pstFib = WanVrf_GetData(uiVrfID, WAN_VRF_PROPERTY_INDEX_FIB);
    WanVrf_SetData(uiVrfID, WAN_VRF_PROPERTY_INDEX_FIB, NULL);

    if (NULL != pstFib)
    {
        RcuEngine_Call(&pstFib->stRcu, _wan_fib_RcuFree);
    }

    return;
}

static FIB_HANDLE _wan_fib_GetFibHandle(IN UINT uiVrfID)
{
    _WAN_FIB_S *pstFib;
    
    pstFib = WanVrf_GetData(uiVrfID, WAN_VRF_PROPERTY_INDEX_FIB);
    if (NULL == pstFib)
    {
        return NULL;
    }

    return pstFib->hFib;
}

static RANGE_FIB_HANDLE _wan_fib_GetRangeFibHandle(IN UINT uiVrfID)
{
    _WAN_FIB_S *pstFib;
    
    pstFib = WanVrf_GetData(uiVrfID, WAN_VRF_PROPERTY_INDEX_FIB);
    if (NULL == pstFib)
    {
        return NULL;
    }

    return pstFib->hRangeFib;
}

static BS_STATUS wan_fib_VFEvent(IN UINT uiEvent, IN UINT uiVrfID, IN USER_HANDLE_S *pstUserHandle)
{
    switch (uiEvent)
    {
        case VF_EVENT_CREATE_VF:
        {
            wan_fib_VFEventCreate(uiVrfID);
            break;
        }

        case VF_EVENT_DESTORY_VF:
        {
            wan_fib_VFEventDestory(uiVrfID);
            break;
        }
    }
    
    return BS_OK;
}

static BS_STATUS _wanfib_AddRouteStatic
(
    IN UINT uiVrf,
    IN UINT uiIP,   /* 网络序 */
    IN UINT uiMask, /* 网络序 */
    IN UINT uiNexthop, /* 网络序 */
    IN VRF_INDEX ifIndex
)
{
    FIB_NODE_S stFibNode;

    memset(&stFibNode, 0, sizeof(stFibNode));

    stFibNode.stFibKey.uiDstOrStartIp = uiIP;
    stFibNode.stFibKey.uiMaskOrEndIp = uiMask;
    stFibNode.uiNextHop = uiNexthop;
    stFibNode.uiOutIfIndex = ifIndex;
    stFibNode.uiFlag = FIB_FLAG_STATIC;
    if (ifIndex == IF_INVALID_INDEX)
    {
        stFibNode.uiFlag |= FIB_FLAG_AUTO_IF;
    }

    return WanFib_Add(uiVrf, &stFibNode);
}

static BS_STATUS _wanfib_ParseIpMaskNethopString
(
    IN CHAR *pcString,
    OUT UINT *puiDstIp,     /* 网络序 */
    OUT UINT *puiMask,      /* 网络序 */
    OUT UINT *puiNexthop    /* 网络序 */
)
{
    LSTR_S stIP;
    LSTR_S stMask;
    LSTR_S stNexthop;
    CHAR szTmp[16];

    TXT_StrSplit(pcString, '_', &stIP, &stMask);
    if ((stIP.uiLen == 0) || (stMask.uiLen == 0))
    {
        return BS_ERR;
    }

    TXT_StrSplit(stMask.pcData, '_', &stMask, &stNexthop);
    if ((stMask.uiLen == 0) || (stNexthop.uiLen == 0))
    {
        return BS_ERR;
    }

    if ((stIP.uiLen > 15) || (stMask.uiLen > 15) || (stNexthop.uiLen > 15))
    {
        return BS_ERR;
    }

    LSTR_Lstr2Str(&stIP, szTmp, sizeof(szTmp));
    *puiDstIp = Socket_Ipsz2IpNet(szTmp);

    LSTR_Lstr2Str(&stMask, szTmp, sizeof(szTmp));
    *puiMask = Socket_Ipsz2IpNet(szTmp);

    LSTR_Lstr2Str(&stNexthop, szTmp, sizeof(szTmp));
    *puiNexthop = Socket_Ipsz2IpNet(szTmp);

    return BS_OK;
}

static BS_STATUS _wanfib_kf_Add(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    CHAR *pcIP;
    CHAR *pcMask;
    CHAR *pcNexthop;

    pcIP = MIME_GetKeyValue(hMime, "IP");
    pcMask = MIME_GetKeyValue(hMime, "Mask");
    pcNexthop = MIME_GetKeyValue(hMime, "Nexthop");
    if ((NULL == pcIP) || (NULL == pcMask) || (NULL == pcNexthop))
    {
        JSON_SetFailed(pstParam->pstJson, "Bad param");
        return BS_OK;
    }

    if (BS_OK != _wanfib_AddRouteStatic(0, Socket_Ipsz2IpNet(pcIP),
            Socket_Ipsz2IpNet(pcMask), Socket_Ipsz2IpNet(pcNexthop), IF_INVALID_INDEX))
    {
        JSON_SetFailed(pstParam->pstJson, "Add route failed");
        return BS_OK;
    }

    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}

static BS_STATUS _wanfib_kf_Del(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    CHAR *pcNames;
    LSTR_S stName;
    FIB_NODE_S stFib;
    CHAR szName[128];

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
            if (BS_OK == _wanfib_ParseIpMaskNethopString(szName,
                &stFib.stFibKey.uiDstOrStartIp,
                &stFib.stFibKey.uiMaskOrEndIp,
                &stFib.uiNextHop))
            {
                WanFib_Del(0, &stFib);
            }            
        }
    }LSTR_SCAN_ELEMENT_END();

    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}

static BS_STATUS _wanfib_kf_List(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    cJSON *pstArray;
    cJSON *pstResJson;
    FIB_HANDLE hFib;
    FIB_NODE_S stFibNode;
    FIB_NODE_S *pstCurrent = NULL;
    CHAR szTmp[128];
    
    hFib = _wan_fib_GetFibHandle(0);

    pstArray = cJSON_CreateArray();
    if (NULL == pstArray)
    {
        JSON_SetFailed(pstParam->pstJson, "Not enough memory");
        return BS_ERR;
    }

    while (BS_OK == FIB_GetNext(hFib, pstCurrent, &stFibNode))
    {
        pstCurrent = &stFibNode;

        if (! (stFibNode.uiFlag & FIB_FLAG_STATIC))
        {
            continue;
        }

        pstResJson = cJSON_CreateObject();
        if (NULL == pstResJson)
        {
            continue;
        }

        BS_Snprintf(szTmp, sizeof(szTmp), "%pI4_%pI4_%pI4",
            &stFibNode.stFibKey.uiDstOrStartIp, &stFibNode.stFibKey.uiMaskOrEndIp, &stFibNode.uiNextHop);
        cJSON_AddStringToObject(pstResJson, "Name", szTmp);

        BS_Snprintf(szTmp, sizeof(szTmp), "%pI4", &stFibNode.stFibKey.uiDstOrStartIp);
        cJSON_AddStringToObject(pstResJson, "IP", szTmp);

        BS_Snprintf(szTmp, sizeof(szTmp), "%pI4", &stFibNode.stFibKey.uiMaskOrEndIp);
        cJSON_AddStringToObject(pstResJson, "Mask", szTmp);

        BS_Snprintf(szTmp, sizeof(szTmp), "%pI4", &stFibNode.uiNextHop);
        cJSON_AddStringToObject(pstResJson, "Nexthop", szTmp);

        cJSON_AddItemToArray(pstArray, pstResJson);
    }

    cJSON_AddItemToObject(pstParam->pstJson, "data", pstArray);

    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}

static BS_WALK_RET_E _wanfib_SaveEach(IN FIB_NODE_S *pstFibNode, IN HANDLE hUserHandle)
{
    UINT uiPrefix;
    UINT uiMask;
    CHAR szIfName[IF_MAX_NAME_LEN + 1] = "";

    if ((pstFibNode->uiFlag & FIB_FLAG_STATIC) == 0)
    {
        return BS_WALK_CONTINUE;
    }

    uiMask = pstFibNode->stFibKey.uiMaskOrEndIp;
    uiMask = ntohl(uiMask);
    uiPrefix = MASK_2_PREFIX(uiMask);

    if (pstFibNode->uiFlag & FIB_FLAG_AUTO_IF)
    {
        CMD_EXP_OutputCmd(hUserHandle, "route static %pI4 %d %pI4",
            &pstFibNode->stFibKey.uiDstOrStartIp, uiPrefix, &pstFibNode->uiNextHop);
    }
    else
    {
        IFNET_Ioctl(pstFibNode->uiOutIfIndex, IFNET_CMD_GET_IFNAME, szIfName);

        CMD_EXP_OutputCmd(hUserHandle, "route static %pI4 %d %pI4 interface %s",
            &pstFibNode->stFibKey.uiDstOrStartIp, uiPrefix, &pstFibNode->uiNextHop, szIfName);
    }

    return BS_WALK_CONTINUE;
}

BS_STATUS WanFib_PrefixMatch(IN UINT uiVrfID, IN UINT uiDstIp /* 网络序 */, OUT FIB_NODE_S *pstFibNode)
{
    BS_STATUS eRet = BS_ERR;
    UINT uiPhase;
    _WAN_FIB_S *pstFib;

    uiPhase = RcuEngine_Lock();    
    pstFib = WanVrf_GetData(uiVrfID, WAN_VRF_PROPERTY_INDEX_FIB);
    if (NULL != pstFib)
    {
        eRet = RangeFib_Match(pstFib->hRangeFib, ntohl(uiDstIp), pstFibNode);
        if (eRet != BS_OK)
        {
            eRet = FIB_PrefixMatch(pstFib->hFib, uiDstIp, pstFibNode);
        }
    }
    RcuEngine_UnLock(uiPhase);
    
    return eRet;
}

PLUG_API BS_STATUS WanFib_AddRange(IN UINT uiVrfID, IN FIB_NODE_S *pstFibNode)
{
    RANGE_FIB_HANDLE hFib;
    BS_STATUS eRet = BS_ERR;
    UINT uiPhase;

    uiPhase = RcuEngine_Lock();
    hFib = _wan_fib_GetRangeFibHandle(uiVrfID);
    if (hFib != NULL)
    {
        eRet = RangeFib_Add(hFib, pstFibNode);
    }
    RcuEngine_UnLock(uiPhase);
    
    return eRet;
}

PLUG_API VOID WanFib_DelRange(IN UINT uiVrfID, IN FIB_KEY_S *pstFibKey)
{
    RANGE_FIB_HANDLE hFib;
    UINT uiPhase;

    uiPhase = RcuEngine_Lock();
    hFib = _wan_fib_GetRangeFibHandle(uiVrfID);
    if (hFib != NULL)
    {
        RangeFib_Del(hFib, pstFibKey);
    }
    RcuEngine_UnLock(uiPhase);
    
    return;
}

PLUG_API BS_STATUS WanFib_Add(IN UINT uiVrfID, IN FIB_NODE_S *pstFibNode)
{
    FIB_HANDLE hFib;
    BS_STATUS eRet = BS_ERR;
    UINT uiPhase;

    uiPhase = RcuEngine_Lock();
    hFib = _wan_fib_GetFibHandle(uiVrfID);
    if (hFib != NULL)
    {
        eRet = FIB_Add(hFib, pstFibNode);
    }
    RcuEngine_UnLock(uiPhase);
    
    return eRet;
}

PLUG_API VOID WanFib_Del(IN UINT uiVrfID, IN FIB_NODE_S *pstFibNode)
{
    FIB_HANDLE hFib;
    UINT uiPhase;

    uiPhase = RcuEngine_Lock();
    hFib = _wan_fib_GetFibHandle(uiVrfID);
    if (hFib != NULL)
    {
        FIB_Del(hFib, pstFibNode);
    }
    RcuEngine_UnLock(uiPhase);
    
    return;
}

BS_STATUS WanFib_Walk(IN UINT uiVrfID, IN PF_FIB_WALK_FUNC pfWalkFunc, IN HANDLE hUserHandle)
{
    FIB_HANDLE hFib;
    BS_STATUS eRet = BS_ERR;
    UINT uiPhase;

    uiPhase = RcuEngine_Lock();
    hFib = _wan_fib_GetFibHandle(uiVrfID);
    if (hFib != NULL)
    {
        FIB_Walk(hFib, pfWalkFunc, hUserHandle);
    }
    RcuEngine_UnLock(uiPhase);
    
    return eRet;
}

BS_STATUS WanFib_Init()
{
    if (BS_OK != WanVrf_RegEventListener(WAN_VRF_REG_PRI_HIGH, wan_fib_VFEvent, NULL))
    {
        return BS_ERR;
    }

    return BS_OK;
}

BS_STATUS WanFib_KfInit()
{
    KFAPP_RegFunc("wan.route.Add", _wanfib_kf_Add, NULL);
    KFAPP_RegFunc("wan.route.Delete", _wanfib_kf_Del, NULL);
    KFAPP_RegFunc("wan.route.List", _wanfib_kf_List, NULL);

	return BS_OK;
}

/*
VF视图下:
show fib
*/
PLUG_API BS_STATUS WAN_FIB_ShowFib
(
    IN UINT ulArgc,
    IN CHAR **argv,
    IN VOID *pEnv
)
{
    UINT uiVrfID;
    FIB_HANDLE hFib;
    RANGE_FIB_HANDLE hRangeFib;
    UINT uiPhase;

    uiVrfID = WAN_VrfCmd_GetVrfByEnv(pEnv);

    uiPhase = RcuEngine_Lock();
    hRangeFib = _wan_fib_GetRangeFibHandle(uiVrfID);
    if (NULL != hRangeFib)
    {
        RangeFib_Show(hRangeFib);
    }
    
    hFib = _wan_fib_GetFibHandle(uiVrfID);
    if (hFib != NULL)
    {
        FIB_Show(hFib);
    }
    RcuEngine_UnLock(uiPhase);

    return BS_OK;
}

VOID WAN_FIB_Save(IN HANDLE hFile)
{
    WanFib_Walk(0, _wanfib_SaveEach, hFile);
}

/*
VF视图下:
route static %IP %INT(prefix) %IP [interface %STRING]
*/
PLUG_API BS_STATUS WAN_FIB_RouteStatic
(
    IN UINT ulArgc,
    IN CHAR **argv,
    IN VOID *pEnv
)
{
    UINT uiPrefix;
    UINT uiMask;
    UINT uiVrfID;
    IF_INDEX ifIndex = IF_INVALID_INDEX;

    uiVrfID = WAN_VrfCmd_GetVrfByEnv(pEnv);

    if (ulArgc >= 8)
    {
        ifIndex = IFNET_GetIfIndex(argv[6]);
        if (IF_INVALID_INDEX == ifIndex)
        {
            EXEC_OutString("Invalid interface name.\r\n");
            return BS_ERR;
        }
    }

    uiPrefix = TXT_Str2Ui(argv[3]);
    uiMask = PREFIX_2_MASK(uiPrefix);

    return _wanfib_AddRouteStatic(uiVrfID, Socket_NameToIpNet(argv[2]), htonl(uiMask), Socket_NameToIpNet(argv[4]), ifIndex);
}

/* no route static %IP(destnation ip address) %INT<0-32>(prefix) %IP(nexthop) */
PLUG_API BS_STATUS WAN_FIB_NoRouteStatic
(
    IN UINT ulArgc,
    IN CHAR **argv,
    IN VOID *pEnv
)
{
    UINT uiPrefix;
    UINT uiMask;
    UINT uiVrfID;
    FIB_NODE_S stFib;

    uiVrfID = WAN_VrfCmd_GetVrfByEnv(pEnv);

    uiPrefix = TXT_Str2Ui(argv[3]);
    uiMask = PREFIX_2_MASK(uiPrefix);

    memset(&stFib, 0, sizeof(stFib));
    stFib.stFibKey.uiDstOrStartIp = Socket_NameToIpNet(argv[3]);
    stFib.stFibKey.uiMaskOrEndIp = htonl(uiMask);
    stFib.uiNextHop = Socket_NameToIpNet(argv[5]);

    WanFib_Del(uiVrfID, &stFib);

	return BS_OK;
}


