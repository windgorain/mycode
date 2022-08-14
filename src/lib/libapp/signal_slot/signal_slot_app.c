/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-3-9
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mutex_utl.h"
#include "utl/signal_slot_utl.h"
#include "comp/comp_signal_slot.h"


static SIG_SLOT_HANDLE g_hSigSlotApp;
static MUTEX_S g_stSigSlotAppMutex;

PLUG_API int SIGSLOT_Connect(CHAR *pcSender, CHAR *pcSignal, CHAR *pcReceiver, 
        PF_SIG_SLOT_RECEIVER_FUNC pfSlotFunc, USER_HANDLE_S *pstSlotUserHandle)
{
    BS_STATUS eRet;
    
    MUTEX_P(&g_stSigSlotAppMutex);
    eRet = SigSlot_Connect(g_hSigSlotApp, pcSender, pcSignal, pcReceiver, pfSlotFunc, pstSlotUserHandle);
    MUTEX_V(&g_stSigSlotAppMutex);

    return eRet;
}

PLUG_API void SIGSLOT_DisConnect(CHAR *pcSender, CHAR *pcSignal, CHAR *pcReceiver, PF_SIG_SLOT_RECEIVER_FUNC pfSlotFunc)
{
    MUTEX_P(&g_stSigSlotAppMutex);
    SigSlot_DisConnect(g_hSigSlotApp, pcSender, pcSignal, pcReceiver, pfSlotFunc);
    MUTEX_V(&g_stSigSlotAppMutex);
}

PLUG_API int SIGSLOT_SendSignal(CHAR *pcSender, CHAR *pcSignal, SIG_SLOT_PARAM_S *pstParam)
{
    BS_STATUS eRet;

    MUTEX_P(&g_stSigSlotAppMutex);
    eRet = SigSlot_SendSignal(g_hSigSlotApp, pcSender, pcSignal, pstParam);
    MUTEX_V(&g_stSigSlotAppMutex);

    return eRet;
}

BS_STATUS SigSlotApp_Init()
{
    g_hSigSlotApp = SigSlot_Create();
    if (NULL == g_hSigSlotApp)
    {
        return BS_ERR;
    }

    MUTEX_Init(&g_stSigSlotAppMutex);

    return BS_OK;
}

