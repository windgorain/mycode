/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2014-1-23
* Description: Keep Alive
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/ka_utl.h"

static VOID ka_KeepAlive(IN KA_S *pstKaConfig)
{
    VCLOCK_INSTANCE_HANDLE hVClockInstance
        = pstKaConfig->stInnerVar.hVClockInstance;

    if (pstKaConfig->stInnerVar.usProbeCount
            >= pstKaConfig->stSet.usMaxProbeCount)
    {
        VCLOCK_Pause(hVClockInstance, &pstKaConfig->stInnerVar.vclock);
        pstKaConfig->stSet.pfKeepAliveFailed(pstKaConfig);
        return;
    }

    pstKaConfig->stInnerVar.usProbeCount ++;

    VCLOCK_RestartWithTick(hVClockInstance, &pstKaConfig->stInnerVar.vclock,
            pstKaConfig->stSet.usIntval, 0);

    pstKaConfig->stSet.pfSendKeepAlive(pstKaConfig);
}

static VOID ka_TimeOut(IN HANDLE hTimerHandle, IN USER_HANDLE_S *pstUserHandle)
{
    KA_S *pstKaConfig = pstUserHandle->ahUserHandle[0];

    ka_KeepAlive(pstKaConfig);

    return;
}

BS_STATUS KA_Start
(
    IN VCLOCK_INSTANCE_HANDLE hVClockInstance,
    IN KA_S *pstKaConfig
)
{
    USER_HANDLE_S stUserHandle;
    UINT tick;
    int ret;

    Mem_Zero(&pstKaConfig->stInnerVar, sizeof(_KA_INNER_VAR_S));

    pstKaConfig->stInnerVar.hVClockInstance = hVClockInstance;

    stUserHandle.ahUserHandle[0] = pstKaConfig;

    if (pstKaConfig->stSet.usIdle == 0) {
        return BS_OK;
    }

    tick = pstKaConfig->stSet.usIdle;

    ret = VCLOCK_AddTimer(hVClockInstance, &pstKaConfig->stInnerVar.vclock,
            tick ,tick, TIMER_FLAG_CYCLE, ka_TimeOut, &stUserHandle);
    if (ret < 0) {
        return BS_ERR;
    }

    return BS_OK;
}

VOID KA_Stop(IN KA_S *pstKaConfig)
{
    VCLOCK_DelTimer(pstKaConfig->stInnerVar.hVClockInstance,
            &pstKaConfig->stInnerVar.vclock);
    pstKaConfig->stInnerVar.usProbeCount = 0;
}

VOID KA_TrigerKeepAlive(IN KA_S *pstKaConfig)
{
    ka_KeepAlive(pstKaConfig);
}

VOID KA_Reset(IN KA_S *pstKaConfig)
{
    UINT tick = pstKaConfig->stSet.usIdle;

    if (tick != 0) {
        VCLOCK_RestartWithTick(pstKaConfig->stInnerVar.hVClockInstance,
                &pstKaConfig->stInnerVar.vclock, tick, 0);
    } else {
        VCLOCK_DelTimer(pstKaConfig->stInnerVar.hVClockInstance,
                &pstKaConfig->stInnerVar.vclock);
    }
    pstKaConfig->stInnerVar.usProbeCount = 0;
}

