/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-11-30
* Description: 
* History:     
******************************************************************************/

#ifndef __WSAPP_WORKER_H_
#define __WSAPP_WORKER_H_

#include "utl/mypoll_utl.h"
#include "comp/comp_wsapp.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef struct
{
    MYPOLL_HANDLE hPollHandle;
    THREAD_ID uiTID;
    UINT uiWorkerID;
    UINT uiConnNum;
    VOID *apListenerData[WSAPP_WORKER_MAX_LISTENER];
}WSAPP_WORKER_S;

BS_STATUS WSAPP_Worker_Init();
WSAPP_WORKER_S * WSAPP_Worker_GetByWorkerID(IN UINT uiWorkerID);
BS_STATUS WSAPP_Worker_ConnDispatch(IN UINT uiGwID, IN INT iSocketID);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__WSAPP_WORKER_H_*/


