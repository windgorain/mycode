/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-1-10
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mbuf_utl.h"
#include "utl/ip_utl.h"
#include "utl/eth_utl.h"
#include "utl/fib_utl.h"
#include "utl/ipfwd_utl.h"
#include "utl/ipfwd_service.h"

typedef struct
{
    DLL_NODE_S stLinkNode;
    UINT uiOrder;
    CHAR *pcName;
    PF_IPFWD_SERVICE_FUNC pfFunc;
    USER_HANDLE_S stUserHandle;
}IPFWD_SERVICE_S;

typedef struct
{
    DLL_HEAD_S astServiceList[IPFWD_SERVICE_PHASE_MAX];
}IPFWD_SERVICE_LIST_S;

#if 0
static CHAR * ipfwd_service_GetRetString(IN IPFWD_SERVICE_RET_E eRet)
{
    CHAR * apcString[IPFWD_SERVICE_RET_MAX] = 
    {
        "Continue",
        "Take Over"
    };

    if (eRet >= IPFWD_SERVICE_RET_MAX)
    {
        return "Ret Error";
    }

    return apcString[eRet];
}
#endif

IPFWD_SERVICE_HANDLE IPFWD_Service_Create()
{
    IPFWD_SERVICE_LIST_S *pstList;
    UINT i;

    pstList = MEM_ZMalloc(sizeof(IPFWD_SERVICE_LIST_S));
    if (NULL == pstList)
    {
        return NULL;
    }

    for (i=0; i<IPFWD_SERVICE_PHASE_MAX; i++)
    {
        DLL_INIT(&pstList->astServiceList[i]);
    }

    return pstList;
}

VOID IPFWD_Service_Destory(IN IPFWD_SERVICE_HANDLE hIpFwdService)
{
    IPFWD_SERVICE_LIST_S *pstList = hIpFwdService;
    UINT i;
    IPFWD_SERVICE_S *pstNode, *pstNodeTmp;

    for (i=0; i<IPFWD_SERVICE_PHASE_MAX; i++)
    {
        DLL_SAFE_SCAN(&pstList->astServiceList[i], pstNode, pstNodeTmp)
        {
            DLL_DEL(&pstList->astServiceList[i], pstNode);
            MEM_Free(pstNode);
        }
    }
}

IPFWD_SERVICE_RET_E IPFWD_Service_Handle
(
    IN IPFWD_SERVICE_HANDLE hIpFwdService,
    IN IPFWD_SERVICE_PHASE_E ePhase,
    IN IP_HEAD_S *pstIpHead,
    IN MBUF_S *pstMbuf
)
{
    IPFWD_SERVICE_RET_E eRet = IPFWD_SERVICE_RET_CONTINUE;
    IPFWD_SERVICE_S *pstService;
    IPFWD_SERVICE_LIST_S *pstList = hIpFwdService;

    BS_DBGASSERT(ePhase < IPFWD_SERVICE_PHASE_MAX);

    DLL_SCAN(&pstList->astServiceList[ePhase], pstService)
    {
        eRet = pstService->pfFunc(pstIpHead, pstMbuf, &pstService->stUserHandle);
        if (eRet != IPFWD_SERVICE_RET_CONTINUE)
        {
            break;
        }
    }

    return eRet;
}

/*
所有的注册必须要在系统正式运行前注册完成.
如果某个系统不需要处理,到自己里面去判断,以免在注册过程中同时报文处理导致死机
*/
BS_STATUS IPFWD_Service_Reg
(
    IN IPFWD_SERVICE_HANDLE hIpFwdService,
    IN IPFWD_SERVICE_PHASE_E ePhase,
    IN UINT uiOrder, /* 优先级. 值越小优先级越高 */
    IN CHAR *pcName, /* 必须静态存在 */
    IN PF_IPFWD_SERVICE_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    IPFWD_SERVICE_S *pstService;
    IPFWD_SERVICE_S *pstNode;
    IPFWD_SERVICE_LIST_S *pstList = hIpFwdService;
    
    if (ePhase >= IPFWD_SERVICE_PHASE_MAX)
    {
        return BS_OUT_OF_RANGE;
    }

    pstService = MEM_ZMalloc(sizeof(IPFWD_SERVICE_S));
    if (NULL == pstService)
    {
        return BS_NO_MEMORY;
    }

    pstService->pcName = pcName;
    pstService->pfFunc = pfFunc;
    if (pstUserHandle != NULL)
    {
        pstService->stUserHandle = *pstUserHandle;
    }

    DLL_SCAN(&pstList->astServiceList[ePhase], pstNode)
    {
        if (pstNode->uiOrder > uiOrder)
        {
            DLL_INSERT_BEFORE(&pstList->astServiceList[ePhase], pstService, pstNode);
            return BS_OK;
        }
    }

    DLL_ADD(&pstList->astServiceList[ePhase], pstService);

    return BS_OK;
}


