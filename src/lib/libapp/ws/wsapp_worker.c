/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-11-30
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
#include "utl/socket_utl.h"

#include "wsapp_def.h"
#include "wsapp_worker.h"
#include "wsapp_gw.h"

static WSAPP_WORKER_S *g_pstWsAppWorker = NULL;
static PF_WSAPP_WORKER_EVENT g_apfWsappWorkerListener[WSAPP_WORKER_MAX_LISTENER] = {0};
static UINT g_uiWsappWorkerNum = WSAPP_WROKER_NUM;

static void wsapp_worker_Main(IN USER_HANDLE_S *pstUserHandle)
{
    WSAPP_WORKER_S *pstWorker = pstUserHandle->ahUserHandle[0];

    MyPoll_Run(pstWorker->hPollHandle);
}

static BS_STATUS wsapp_worker_InitWorker(IN WSAPP_WORKER_S *pstWorker)
{
    USER_HANDLE_S stUserHandle;
    
    pstWorker->hPollHandle = MyPoll_Create();
    if (pstWorker->hPollHandle == NULL)
    {
        return BS_ERR;
    }

    stUserHandle.ahUserHandle[0] = pstWorker;

    pstWorker->uiTID = THREAD_Create("WsWorker", NULL, wsapp_worker_Main, &stUserHandle);
    if (THREAD_ID_INVALID == pstWorker->uiTID) {
        MyPoll_Destory(pstWorker->hPollHandle);
        pstWorker->hPollHandle = NULL;
        return BS_ERR;
    }

    return BS_OK;
}

BS_STATUS WSAPP_Worker_Init()
{
    UINT i;
    
    g_pstWsAppWorker = MEM_ZMalloc(sizeof(WSAPP_WORKER_S) * g_uiWsappWorkerNum);
    if (NULL == g_pstWsAppWorker)
    {
        return BS_ERR;
    }

    for (i=0; i<g_uiWsappWorkerNum; i++)
    {
        g_pstWsAppWorker[i].uiWorkerID = i;
        wsapp_worker_InitWorker(&g_pstWsAppWorker[i]);
    }

    return BS_OK;
}

WSAPP_WORKER_S * WSAPP_Worker_GetByWorkerID(IN UINT uiWorkerID)
{
    if (uiWorkerID >= g_uiWsappWorkerNum)
    {
        return NULL;
    }

    return &g_pstWsAppWorker[uiWorkerID];
}

BS_STATUS WSAPP_Worker_ConnDispatch(IN UINT uiGwID, IN INT iSocketID)
{
    UINT i;
    UINT uiConnNum = 0xffffffff;
    WSAPP_WORKER_S *pstWorker = NULL;
    CONN_HANDLE hConn;
    BS_STATUS eRet;

    for (i=0; i<g_uiWsappWorkerNum; i++)
    {
        if ((g_pstWsAppWorker[i].uiTID != 0) && (g_pstWsAppWorker[i].uiConnNum < uiConnNum))
        {
            pstWorker = &g_pstWsAppWorker[i];
            uiConnNum = pstWorker->uiConnNum;
        }
    }

    if (pstWorker == NULL)
    {
        Socket_Close(iSocketID);
        return BS_ERR;
    }

    hConn = CONN_New(iSocketID);
    if (NULL == hConn)
    {
        Socket_Close(iSocketID);
        return BS_ERR;
    }

    CONN_SetPoller(hConn, pstWorker->hPollHandle);
    CONN_SetUserData(hConn, CONN_USER_DATA_INDEX_0, UINT_HANDLE(pstWorker->uiWorkerID));

    eRet = WSAPP_GW_NewConn(uiGwID, hConn);

    return eRet;
}

UINT WSAPP_Worker_RegListener(IN PF_WSAPP_WORKER_EVENT pfEventFunc)
{
    UINT i;
    UINT uiListenerID = WSAPP_WORKER_LISTENER_ID_INVALID;

    for (i=0; i<WSAPP_WORKER_MAX_LISTENER; i++)
    {
        if (g_apfWsappWorkerListener[i] == NULL)
        {
            g_apfWsappWorkerListener[i] = pfEventFunc;
            uiListenerID = i;
            break;
        }
    }

    for (i=0; i<g_uiWsappWorkerNum; i++)
    {
        pfEventFunc(i, WSAPP_WORKER_EVENT_CREATE);
    }

    return uiListenerID;
}

BS_STATUS WSAPP_Worker_SetListenerData(IN UINT uiListenerID, IN UINT uiWorkerID, IN VOID *pData)
{
    if (uiListenerID >= WSAPP_WORKER_MAX_LISTENER)
    {
        BS_DBGASSERT(0);
        return BS_BAD_PARA;
    }

    if (uiWorkerID >= g_uiWsappWorkerNum)
    {
        BS_DBGASSERT(0);
        return BS_BAD_PARA;
    }

    g_pstWsAppWorker[uiWorkerID].apListenerData[uiListenerID] = pData;

    return BS_OK;
}

VOID * WSAPP_Worker_GetListenerData(IN UINT uiListenerID, IN UINT uiWorkerID)
{
    if (uiListenerID >= WSAPP_WORKER_MAX_LISTENER)
    {
        BS_DBGASSERT(0);
        return NULL;
    }

    if (uiWorkerID >= g_uiWsappWorkerNum)
    {
        return NULL;
    }

    return g_pstWsAppWorker[uiWorkerID].apListenerData[uiListenerID];
}
