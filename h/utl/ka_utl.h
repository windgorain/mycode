/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-1-23
* Description: 
* History:     
******************************************************************************/

#ifndef __KA_UTL_H_
#define __KA_UTL_H_

#include "utl/vclock_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

struct tag_KA;

typedef VOID (*PF_KA_SEND_KEEP_ALIVE)(IN struct tag_KA *pstKa);
typedef VOID (*PF_KA_FAILED)(IN struct tag_KA *pstKa);

typedef struct
{
    USHORT usIdle;   /* 空闲Tick数目,这么多tick之后开始触发探测,0表示永不触发*/
    USHORT usIntval; /* 探测间隔Tick */
    USHORT usMaxProbeCount; /* 最大探测次数 */
    PF_KA_SEND_KEEP_ALIVE pfSendKeepAlive;
    PF_KA_FAILED pfKeepAliveFailed;
}KA_SET_S;

typedef struct
{
    VCLOCK_INSTANCE_HANDLE hVClockInstance;
    USHORT usProbeCount; /* 已经探测过几次连续失败了 */
    VCLOCK_NODE_S vclock;
}_KA_INNER_VAR_S;

typedef struct tag_KA
{
    KA_SET_S stSet; /* 需要使用者填写 */
    USER_HANDLE_S stUserHandle;
    _KA_INNER_VAR_S stInnerVar; /* KA内部使用的变量,使用者不需要关心 */
}KA_S;


BS_STATUS KA_Start
(
    IN VCLOCK_INSTANCE_HANDLE hVClockInstance,
    IN KA_S *pstKaConfig
);
VOID KA_TrigerKeepAlive(IN KA_S *pstKaConfig);
VOID KA_Stop(IN KA_S *pstKaConfig);
VOID KA_Reset(IN KA_S *pstKaConfig);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__KA_UTL_H_*/


