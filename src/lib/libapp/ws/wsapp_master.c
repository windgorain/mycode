/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-11-30
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mypoll_utl.h"
#include "utl/exec_utl.h"
#include "utl/socket_utl.h"

#include "wsapp_def.h"
#include "wsapp_gw.h"
#include "wsapp_worker.h"

static THREAD_ID g_uiWsAppMasterTID = 0;
static MYPOLL_HANDLE g_hWsAppMasterPoll = NULL;

static BS_WALK_RET_E wsapp_master_Accept(IN INT iSocketId, IN UINT uiEvent, IN USER_HANDLE_S *pstUserHandle)
{
    INT iAccetpSocketId;
    UINT uiGwID;

    uiGwID = (UINT)(ULONG)pstUserHandle->ahUserHandle[0];

    if (uiEvent & MYPOLL_EVENT_IN)
    {
        iAccetpSocketId = Socket_Accept(iSocketId, NULL, NULL);
        if (iAccetpSocketId < 0)
        {
            return BS_WALK_CONTINUE;
        }

        if (TRUE != WSAPP_GW_IsFilterPermit(uiGwID, iAccetpSocketId))
        {
            Socket_Close(iAccetpSocketId);
        }
        else
        {
            Socket_SetNoBlock(iAccetpSocketId, TRUE);
            WSAPP_Worker_ConnDispatch(uiGwID, iAccetpSocketId);
        }
    }

    return BS_WALK_CONTINUE;
}

static void wsapp_master_Main(IN USER_HANDLE_S *pstUserHandle)
{
    while (1)
    {
        MyPoll_Run(g_hWsAppMasterPoll);
    }
}

BS_STATUS WSAPP_Master_Init()
{
    g_hWsAppMasterPoll = MyPoll_Create();
    if (g_hWsAppMasterPoll == NULL)
    {
        return BS_ERR;
    }
    
    g_uiWsAppMasterTID = THREAD_Create("WsMaster", NULL, wsapp_master_Main, NULL);
    if (THREAD_ID_INVALID == g_uiWsAppMasterTID) {
        return BS_ERR;
    }

    return BS_OK;
}

BS_STATUS WSAPP_Master_AddGW(IN WSAPP_GW_S *pstGW, IN UINT uiIp, IN USHORT usPort)
{
    USER_HANDLE_S stUserHandle;
    INT iListenSocketId;
    UINT ulIoMode =1;

    iListenSocketId = Socket_Create(AF_INET, SOCK_STREAM);
    if (iListenSocketId < 0)
    {
        return BS_NO_RESOURCE;
    }

    Socket_Ioctl(iListenSocketId, (INT) FIONBIO, &ulIoMode);

    if (BS_OK != Socket_Listen(iListenSocketId, htonl(uiIp), htons(usPort), 5))
    {
        EXEC_OutInfo(" Can not listen port %d.\r\n", usPort);
        Socket_Close(iListenSocketId);
        return BS_CONFLICT;
    }

    stUserHandle.ahUserHandle[0] = UINT_HANDLE(WSAPP_GW_GetID(pstGW));

    if (BS_OK != MyPoll_SetEvent(g_hWsAppMasterPoll,
                            iListenSocketId,
                            MYPOLL_EVENT_IN | MYPOLL_EVENT_ERR,
                            wsapp_master_Accept,
                            &stUserHandle))
    {
        Socket_Close(iListenSocketId);
        return (BS_ERR);
    }

    pstGW->iListenSocket = iListenSocketId;

    return BS_OK;
}

VOID WSAPP_Master_DelGW(IN WSAPP_GW_S *pstGW)
{
    if (pstGW->iListenSocket >= 0)
    {
        Socket_Close(pstGW->iListenSocket);
        pstGW->iListenSocket = -1;
    }
}

