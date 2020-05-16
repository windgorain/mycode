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
static COMP_SigSlotApp_S g_stSigSlotAppComp;
static MUTEX_S g_stSigSlotAppMutex;

static BS_STATUS sigslotapp_Connect
(
    IN CHAR *pcSender,
    IN CHAR *pcSignal,
    IN CHAR *pcReceiver,
    IN PF_SIG_SLOT_RECEIVER_FUNC pfSlotFunc,
    IN USER_HANDLE_S *pstSlotUserHandle
)
{
    BS_STATUS eRet;
    
    MUTEX_P(&g_stSigSlotAppMutex);
    eRet = SigSlot_Connect(g_hSigSlotApp, pcSender, pcSignal, pcReceiver, pfSlotFunc, pstSlotUserHandle);
    MUTEX_V(&g_stSigSlotAppMutex);

    return eRet;
}

static VOID sigslotapp_DisConnect
(
    IN CHAR *pcSender,
    IN CHAR *pcSignal,
    IN CHAR *pcReceiver,
    IN PF_SIG_SLOT_RECEIVER_FUNC pfSlotFunc
)
{
    MUTEX_P(&g_stSigSlotAppMutex);
    SigSlot_DisConnect(g_hSigSlotApp, pcSender, pcSignal, pcReceiver, pfSlotFunc);
    MUTEX_V(&g_stSigSlotAppMutex);
}

static BS_STATUS sigslotapp_SendSignal
(
    IN CHAR *pcSender,
    IN CHAR *pcSignal,
    IN SIG_SLOT_PARAM_S *pstParam
)
{
    BS_STATUS eRet;

    MUTEX_P(&g_stSigSlotAppMutex);
    eRet = SigSlot_SendSignal(g_hSigSlotApp, pcSender, pcSignal, pstParam);
    MUTEX_V(&g_stSigSlotAppMutex);

    return eRet;
}

static VOID sigslotapp_InitComp()
{
    g_stSigSlotAppComp.pfConnect = sigslotapp_Connect;
    g_stSigSlotAppComp.pfDisConnect = sigslotapp_DisConnect;
    g_stSigSlotAppComp.pfSendSignal = sigslotapp_SendSignal;
    g_stSigSlotAppComp.comp.comp_name = COMP_SIGSLOT_APP_NAME;

    COMP_Reg(&g_stSigSlotAppComp.comp);
}

BS_STATUS SigSlotApp_Init()
{
    g_hSigSlotApp = SigSlot_Create();
    if (NULL == g_hSigSlotApp)
    {
        return BS_ERR;
    }

    MUTEX_Init(&g_stSigSlotAppMutex);

    sigslotapp_InitComp();

    return BS_OK;
}

