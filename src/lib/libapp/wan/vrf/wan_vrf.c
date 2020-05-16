/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2013-1-24
* Description: 虚拟平面
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/sif_utl.h"
#include "utl/fib_utl.h"
#include "utl/ipfwd_service.h"
#include "utl/arp_utl.h"
#include "utl/txt_utl.h"
#include "utl/mutex_utl.h"
#include "utl/object_utl.h"
#include "utl/ob_chain.h"

#include "../h/wan_vrf.h"
#include "../h/wan_ifnet.h"
#include "../h/wan_ipfwd_service.h"
#include "../h/wan_fib.h"
#include "../h/wan_ipfwd.h"
#include "../h/wan_blackhole.h"


static NO_HANDLE g_hWanVrf;
static MTIMER_S g_stWanVrfMTimer;
static MUTEX_S g_stWanVrfMutex;
static UINT g_uiWanVrfDefault = 0;
static OB_CHAIN_S g_stWanVrfObserver = OB_CHAIN_HEAD_INIT_VALUE(&g_stWanVrfObserver);

static VOID _wan_vrf_EventNotify(IN WAN_VRF_EVENT_E enEvent, IN UINT uiVrfID)
{
    OB_CHAIN_NOTIFY2(&g_stWanVrfObserver, enEvent, uiVrfID);
}

static VOID wan_vrf_TimeOut(IN HANDLE hTimerHandle, IN USER_HANDLE_S *pstUserHandle)
{
    UINT uiVrfId = 0;

    while ((uiVrfId = WanVrf_GetNext(uiVrfId)) != 0)
    {
        _wan_vrf_EventNotify(WAN_VRF_EVENT_TIMER, uiVrfId);
    }
}

BS_STATUS WanVrf_Init()
{
    int ret;

    MUTEX_Init(&g_stWanVrfMutex);

    g_hWanVrf = NO_CreateAggregate(WAN_MAX_VRF, 0, 0);
    if (NULL == g_hWanVrf)
    {
        RETURN(BS_ERR);
    }

    NO_EnableSeq(g_hWanVrf, 0xffff0000, 1);

    ret = MTimer_Add(&g_stWanVrfMTimer, WAN_VRF_TIME, TIMER_FLAG_CYCLE,
            wan_vrf_TimeOut, NULL);
    if (ret < 0) {
        RETURN(BS_ERR);
    }

    return BS_OK;
}

BS_STATUS WanVrf_Init2()
{
    /* 创建一个缺省VRF */
    g_uiWanVrfDefault = WanVrf_CreateVrf("default");
    if (0 == g_uiWanVrfDefault)
    {
        return BS_ERR;
    }

    return BS_OK;
}

UINT WAN_VRF_GetDefaultVrf()
{
    return g_uiWanVrfDefault;
}

/* 返回0表示失败 */
UINT WanVrf_CreateVrf(IN CHAR *pcVrfName)
{
    UINT uiVrfID;

    BS_DBGASSERT(NULL != pcVrfName);

    if (strlen(pcVrfName) > WAN_VRF_MAX_NAME_LEN)
    {
        return 0;
    }

    MUTEX_P(&g_stWanVrfMutex);
    uiVrfID = (UINT)NO_NewObjectID(g_hWanVrf, pcVrfName);
    MUTEX_V(&g_stWanVrfMutex);

    if (uiVrfID != 0)
    {
        _wan_vrf_EventNotify(WAN_VRF_EVENT_CREATE, uiVrfID);
    }

    return uiVrfID;
}

VOID WanVrf_DestoryVrf(IN UINT uiVrfID)
{
    if (g_uiWanVrfDefault == uiVrfID)
    {
        return;
    }

    _wan_vrf_EventNotify(WAN_VRF_EVENT_DESTROY, uiVrfID);

    MUTEX_P(&g_stWanVrfMutex);
    NO_FreeObjectByID(g_hWanVrf, uiVrfID);
    MUTEX_V(&g_stWanVrfMutex);
}

BS_STATUS WanVrf_RegEventListener
(
    IN UINT uiPriority,
    IN PF_WAN_VRF_EVENT_FUNC pfEventFunc,
    IN USER_HANDLE_S * pstUserHandle
)
{
    BS_STATUS eRet;

    MUTEX_P(&g_stWanVrfMutex);
    eRet = OB_CHAIN_AddWithPri(&g_stWanVrfObserver, uiPriority, (UINT_FUNC_X)pfEventFunc, pstUserHandle);
    MUTEX_V(&g_stWanVrfMutex);

    return eRet;
}

BS_STATUS WanVrf_SetData(IN UINT uiVrfID, IN UINT enIndex, IN HANDLE hData)
{
    BS_STATUS eRet;

    if (uiVrfID == 0)
    {
        uiVrfID = g_uiWanVrfDefault;
    }

    MUTEX_P(&g_stWanVrfMutex);
    eRet = NO_SetPropertyByID(g_hWanVrf, uiVrfID, enIndex, hData);
    MUTEX_V(&g_stWanVrfMutex);

    return eRet;
}

HANDLE WanVrf_GetData(IN UINT uiVrfID, IN UINT enIndex)
{
    HANDLE hHandle = NULL;

    if (uiVrfID == 0)
    {
        uiVrfID = g_uiWanVrfDefault;
    }

    MUTEX_P(&g_stWanVrfMutex);
    NO_GetPropertyByID(g_hWanVrf, uiVrfID, enIndex, &hHandle);
    MUTEX_V(&g_stWanVrfMutex);

    return hHandle;
}

UINT WanVrf_GetIdByName(IN CHAR *pcName)
{
    VOID *pObj;
    UINT uiVrfID;

    MUTEX_P(&g_stWanVrfMutex);
    pObj = NO_GetObjectByName(g_hWanVrf, pcName);
    uiVrfID = (UINT)NO_GetObjectID(g_hWanVrf, pObj);
    MUTEX_V(&g_stWanVrfMutex);
    
    return uiVrfID;
}

BS_STATUS WanVrf_GetNameByID(IN UINT uiVrfID, OUT CHAR szName[WAN_VRF_MAX_NAME_LEN + 1])
{
    CHAR *pcName;
    BS_STATUS eRet = BS_NOT_FOUND;

    if (uiVrfID == 0)
    {
        uiVrfID = g_uiWanVrfDefault;
    }

    MUTEX_P(&g_stWanVrfMutex);
    pcName = NO_GetNameByID(g_hWanVrf, uiVrfID);
    if (NULL != pcName)
    {
        eRet = BS_OK;
        TXT_Strlcpy(szName, pcName, WAN_VRF_MAX_NAME_LEN + 1);
    }
    MUTEX_V(&g_stWanVrfMutex);

    return eRet;
}

/* 相比于WAN_VRF_GetNameByID, 直接返回字符串 */
CHAR * WanVrf_GetNameByID2(IN UINT uiVrfID, OUT CHAR szName[WAN_VRF_MAX_NAME_LEN + 1])
{
    if (uiVrfID == 0)
    {
        uiVrfID = g_uiWanVrfDefault;
    }

    if (BS_OK != WanVrf_GetNameByID(uiVrfID, szName))
    {
        return "";
    }

    return szName;
}

UINT WanVrf_GetNext(IN UINT uiCurrentVrf)
{
    UINT ulNext;

    MUTEX_P(&g_stWanVrfMutex);
    ulNext = (UINT)NO_GetNextID(g_hWanVrf, uiCurrentVrf);
    MUTEX_V(&g_stWanVrfMutex);

    return ulNext;
}

/* 自动分配一个DataIndex */
UINT WanVrf_AllocDataIndex()
{
    static UINT uiVrfIndex = WAN_VRF_PROPERTY_INDEX_MAX;
    UINT uiRet;

    MUTEX_P(&g_stWanVrfMutex);
    uiRet = uiVrfIndex;
    uiVrfIndex ++;
    MUTEX_V(&g_stWanVrfMutex);

    return uiRet;
}

