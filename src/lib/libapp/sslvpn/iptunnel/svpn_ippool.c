/******************************************************************************
* Copyright (C), 2000-2006,  Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2016-7-17
* Description: svpn iptunnel 虚拟地址池
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mutex_utl.h"
#include "utl/socket_utl.h"
#include "utl/ippool_utl.h"
#include "utl/ip_list.h"
#include "utl/my_ip_helper.h"
#include "app/wan_pub.h"
#include "app/if_pub.h"
#include "comp/comp_wan.h"

#include "../h/svpn_def.h"
#include "../h/svpn_context.h"
#include "../h/svpn_ctxdata.h"
#include "../h/svpn_dweb.h"
#include "../h/svpn_mf.h"
#include "../h/svpn_ippool.h"
#include "../h/svpn_iptunnel.h"

typedef struct
{
    MUTEX_S stMutex;
    IPPOOL_HANDLE hIpPool;
}_SVPN_IPPOOL_CTRL_S;

_SVPN_IPPOOL_CTRL_S * _svpn_ippool_GetCtrl(IN SVPN_CONTEXT_HANDLE hSvpnContext)
{
    return SVPN_Context_GetUserData(hSvpnContext, SVPN_CONTEXT_DATA_INDEX_IPTUN_IPPOOL);
}

static BS_STATUS _svpn_ippool_Restore(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcName)
{
    CHAR szStartIP[16];
    CHAR szEndIP[16];
    UINT uiStartIP;
    UINT uiEndIP;

    if (BS_OK != SVPN_CtxData_GetProp(hSvpnContext, SVPN_CTXDATA_IPPOOL, pcName,
        "StartIP", szStartIP, sizeof(szStartIP)))
    {
        return BS_ERR;
    }

    if (BS_OK != SVPN_CtxData_GetProp(hSvpnContext, SVPN_CTXDATA_IPPOOL, pcName,
        "EndIP", szEndIP, sizeof(szEndIP)))
    {
        return BS_ERR;
    }

    uiStartIP = Socket_NameToIpHost(szStartIP);
    uiEndIP = Socket_NameToIpHost(szEndIP);

    return SVPN_IPPOOL_AddRange(hSvpnContext, uiStartIP, uiEndIP);
}

static BS_STATUS _svpn_ippool_ContextCreate(IN SVPN_CONTEXT_HANDLE hSvpnContext)
{
    _SVPN_IPPOOL_CTRL_S *pstIpPoolCtrl;
    CHAR szName[512] = "";

    pstIpPoolCtrl = MEM_ZMalloc(sizeof(_SVPN_IPPOOL_CTRL_S));
    if (NULL == pstIpPoolCtrl)
    {
        return BS_NO_MEMORY;
    }

    pstIpPoolCtrl->hIpPool = IPPOOL_Create();
    if (NULL == pstIpPoolCtrl->hIpPool)
    {
        MEM_Free(pstIpPoolCtrl);
        return BS_NO_MEMORY;
    }

    MUTEX_Init(&pstIpPoolCtrl->stMutex);

    SVPN_Context_SetUserData(hSvpnContext, SVPN_CONTEXT_DATA_INDEX_IPTUN_IPPOOL, pstIpPoolCtrl);

    /* 恢复配置 */
    while (BS_OK == SVPN_CtxData_GetNextObject(hSvpnContext,
        SVPN_CTXDATA_IPPOOL, szName, szName, sizeof(szName)))
    {
        _svpn_ippool_Restore(hSvpnContext, szName);
    }

    return BS_OK;
}

static BS_STATUS _svpn_ippool_ContextDestroy(IN SVPN_CONTEXT_HANDLE hSvpnContext)
{
    _SVPN_IPPOOL_CTRL_S *pstCtrl;

    pstCtrl = _svpn_ippool_GetCtrl(hSvpnContext);
    if (NULL != pstCtrl)
    {
        IPPOOL_Destory(pstCtrl->hIpPool);
        MUTEX_Final(&pstCtrl->stMutex);
        MEM_Free(pstCtrl);
        SVPN_Context_SetUserData(hSvpnContext, SVPN_CONTEXT_DATA_INDEX_ULM, NULL);
    }

    return BS_OK;
}

static BS_STATUS _svpn_ippool_ContextEvent(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN UINT uiEvent)
{
    BS_STATUS eRet = BS_OK;

    switch (uiEvent)
    {
        case SVPN_CONTEXT_EVENT_CREATE:
        {
            _svpn_ippool_ContextCreate(hSvpnContext);
            break;
        }

        case SVPN_CONTEXT_EVENT_DESTROY:
        {
            _svpn_ippool_ContextDestroy(hSvpnContext);
            break;
        }

        default:
        {
            break;
        }
    }

    return eRet;
}

BS_STATUS SVPN_IPPOOL_Init()
{
    SVPN_Context_RegEvent(_svpn_ippool_ContextEvent);

    return BS_OK;
}

static BS_STATUS svpn_ippool_AddFibRange
(
    IN UINT uiStartIP,
    IN UINT uiEndIP
)
{
    FIB_NODE_S stFibNode;

    memset(&stFibNode, 0, sizeof(stFibNode));

    stFibNode.uiOutIfIndex = SVPN_IpTunnel_GetInterface();
    stFibNode.stFibKey.uiDstOrStartIp = uiStartIP;
    stFibNode.stFibKey.uiMaskOrEndIp = uiEndIP;

    WanFib_AddRange(0, &stFibNode);

    return BS_OK;
}

static BS_STATUS svpn_ippool_DelFibRange
(
    IN UINT uiStartIP,
    IN UINT uiEndIP
)
{
    FIB_KEY_S stFibKey = {0};

    stFibKey.uiDstOrStartIp = uiStartIP;
    stFibKey.uiMaskOrEndIp = uiEndIP;

    WanFib_DelRange(0, &stFibKey);

    return BS_OK;
}

BS_STATUS SVPN_IPPOOL_AddRange
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN UINT uiStartIP/* 主机序 */,
    IN UINT uiEndIP/* 主机序 */
)
{
    _SVPN_IPPOOL_CTRL_S *pstCtrl;
    BS_STATUS eRet;
    FIB_NODE_S stFibNode;

    memset(&stFibNode, 0, sizeof(stFibNode));

    pstCtrl = _svpn_ippool_GetCtrl(hSvpnContext);
    if (NULL == pstCtrl)
    {
        return BS_ERR;
    }

    MUTEX_P(&pstCtrl->stMutex);
    eRet = IPPOOL_AddRange(pstCtrl->hIpPool, uiStartIP, uiEndIP);
    MUTEX_V(&pstCtrl->stMutex);

    if (BS_OK == eRet)
    {
        svpn_ippool_AddFibRange(uiStartIP, uiEndIP);
    }

    return eRet;
}

VOID SVPN_IPPOOL_DelRange
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN UINT uiStartIP/* 主机序 */,
    IN UINT uiEndIP/* 主机序 */
)
{
    _SVPN_IPPOOL_CTRL_S *pstCtrl;
    BS_STATUS eRet;

    pstCtrl = _svpn_ippool_GetCtrl(hSvpnContext);
    if (NULL == pstCtrl)
    {
        return;
    }

    MUTEX_P(&pstCtrl->stMutex);
    eRet = IPPOOL_DelRange(pstCtrl->hIpPool, uiStartIP, uiEndIP);
    MUTEX_V(&pstCtrl->stMutex);

    if (BS_OK == eRet)
    {
        svpn_ippool_DelFibRange(uiStartIP, uiEndIP);
    }

    return;
}

BS_STATUS SVPN_IPPOOL_ModifyRange
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN UINT uiOldStartIP/* 主机序 */,
    IN UINT uiOldEndIP/* 主机序 */,
    IN UINT uiStartIP/* 主机序 */,
    IN UINT uiEndIP/* 主机序 */
)
{
    _SVPN_IPPOOL_CTRL_S *pstCtrl;
    BS_STATUS eRet;

    if ((uiOldStartIP == uiStartIP) && (uiOldEndIP == uiEndIP))
    {
        return BS_OK;
    }

    pstCtrl = _svpn_ippool_GetCtrl(hSvpnContext);
    if (NULL == pstCtrl)
    {
        return BS_ERR;
    }

    MUTEX_P(&pstCtrl->stMutex);
    eRet = IPPOOL_ModifyRange(pstCtrl->hIpPool, uiOldStartIP, uiOldEndIP, uiStartIP, uiEndIP);
    MUTEX_V(&pstCtrl->stMutex);

    if (BS_OK == eRet)
    {
        svpn_ippool_DelFibRange(uiOldStartIP, uiOldEndIP);
        svpn_ippool_AddFibRange(uiStartIP, uiEndIP);
    }

    return eRet;
}

/* 申请一个IP，返回主机序IP */
UINT SVPN_IPPOOL_AllocIP(IN SVPN_CONTEXT_HANDLE hSvpnContext)
{
    _SVPN_IPPOOL_CTRL_S *pstCtrl;
    UINT uiIP;

    pstCtrl = _svpn_ippool_GetCtrl(hSvpnContext);
    if (NULL == pstCtrl)
    {
        return 0;
    }

    MUTEX_P(&pstCtrl->stMutex);
    uiIP = IPPOOL_AllocIP(pstCtrl->hIpPool, 0);
    MUTEX_V(&pstCtrl->stMutex);

    return uiIP;
}

VOID SVPN_IPPOOL_FreeIP(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN UINT uiIP/* 主机序 */)
{
    _SVPN_IPPOOL_CTRL_S *pstCtrl;

    pstCtrl = _svpn_ippool_GetCtrl(hSvpnContext);
    if (NULL == pstCtrl)
    {
        return;
    }

    MUTEX_P(&pstCtrl->stMutex);
    IPPOOL_FreeIP(pstCtrl->hIpPool, uiIP);
    MUTEX_V(&pstCtrl->stMutex);

    return;
}


