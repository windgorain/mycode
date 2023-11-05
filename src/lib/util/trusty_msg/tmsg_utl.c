/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-5-27
* Description: trusty msg,  可靠地消息发送/接收机制, 比如用于UDP以使其可靠
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/vclock_utl.h"
#include "utl/tmsg_utl.h"

typedef struct
{
    VCLOCK_INSTANCE_HANDLE hVclock;
    DLL_HEAD_S stMsgList;
    UINT uiSequence;
}TMSG_CTRL_S;

typedef struct
{
    DLL_NODE_S stListNode;

    TMSG_OPT_S stOpt;
    USER_HANDLE_S stUserHandle;

    UINT uiCount;
    VCLOCK_HANDLE hVclockId;
}TMSG_NODE_S;

TMSG_HANDLE TMSG_Create()
{
    TMSG_CTRL_S *pstCtrl;

    pstCtrl = MEM_ZMalloc(sizeof(TMSG_CTRL_S));
    if (NULL == pstCtrl)
    {
        return NULL;
    }

    pstCtrl->hVclock = VCLOCK_CreateInstance(TRUE);
    if (NULL == pstCtrl->hVclock)
    {
        TMSG_Destory(pstCtrl);
        return NULL;
    }

    DLL_INIT(&pstCtrl->stMsgList);

    return pstCtrl;
}

VOID TMSG_Destory(IN TMSG_HANDLE hTmsgHandle)
{
    TMSG_CTRL_S *pstCtrl = hTmsgHandle;

    if (pstCtrl != NULL)
    {
        TMSG_StopAll(hTmsgHandle);

        if (pstCtrl->hVclock != NULL)
        {
            VCLOCK_DeleteInstance(pstCtrl->hVclock);
        }

        MEM_Free(pstCtrl);
    }
}

static VOID tmsg_FreeNode(IN TMSG_CTRL_S *pstCtrl, IN TMSG_NODE_S *pstNode)
{
    DLL_DEL(&pstCtrl->stMsgList, pstNode);
    MEM_Free(pstNode);
}

static VOID tmsg_TimeOut(IN HANDLE hTimerId, IN USER_HANDLE_S *pstUserHandle)
{
    TMSG_CTRL_S *pstCtrl = pstUserHandle->ahUserHandle[0];
    TMSG_NODE_S *pstNode = pstUserHandle->ahUserHandle[1];

    pstNode->uiCount ++;

    if ((pstNode->stOpt.uiRetryTimes == 0)
        || (pstNode->uiCount <= pstNode->stOpt.uiRetryTimes))
    {
        pstNode->stOpt.pfEventFunc(TMSG_EVENT_SEND, &pstNode->stOpt, &pstNode->stUserHandle);
    }
    else if (pstNode->uiCount > pstNode->stOpt.uiRetryTimes)
    {
        
        VCLOCK_DestroyTimer(pstCtrl->hVclock, hTimerId);

        if (pstNode->stOpt.stTmsg.uiFlag & TMSG_FLAG_NEED_ACK)
        {
            pstNode->stOpt.pfEventFunc(TMSG_EVENT_FAILED, &pstNode->stOpt, &pstNode->stUserHandle);
        }
        else
        {
            pstNode->stOpt.pfEventFunc(TMSG_EVENT_SUCCESS, &pstNode->stOpt, &pstNode->stUserHandle);
        }

        tmsg_FreeNode(pstCtrl, pstNode);
    }
}

BS_STATUS TMSG_Send
(
    IN TMSG_HANDLE hTmsgHandle,
    IN TMSG_OPT_S *pstOpt,
    IN USER_HANDLE_S *pstUserHandle
)
{
    TMSG_CTRL_S *pstCtrl = hTmsgHandle;
    TMSG_NODE_S *pstNode;
    USER_HANDLE_S stUserHandle;

    pstNode = MEM_ZMalloc(sizeof(TMSG_NODE_S));
    if (NULL == pstNode)
    {
        pstOpt->pfEventFunc(TMSG_EVENT_FAILED, pstOpt, pstUserHandle);
        return BS_NO_MEMORY;
    }

    pstNode->stOpt = *pstOpt;
    pstNode->stOpt.stTmsg.uiSequence = pstCtrl->uiSequence++;

    if (NULL != pstUserHandle)
    {
        pstNode->stUserHandle = *pstUserHandle;
    }

    pstNode->uiCount ++;  

    DLL_ADD(&pstCtrl->stMsgList, pstNode);

    stUserHandle.ahUserHandle[0] = hTmsgHandle;
    stUserHandle.ahUserHandle[1] = pstNode;

    pstNode->hVclockId = VCLOCK_CreateTimer(pstCtrl->hVclock, pstOpt->uiTicks, pstOpt->uiTicks, TIMER_FLAG_CYCLE, tmsg_TimeOut, &stUserHandle);
    if (pstNode->hVclockId == NULL)
    {
        pstOpt->pfEventFunc(TMSG_EVENT_FAILED, &pstNode->stOpt, pstUserHandle);
        tmsg_FreeNode(pstCtrl, pstNode);
        return BS_ERR;
    }

    pstOpt->pfEventFunc(TMSG_EVENT_SEND, &pstNode->stOpt, pstUserHandle);

    return BS_OK;
}

UINT TMSG_GetMsgNum(IN TMSG_HANDLE hTmsgHandle)
{
    TMSG_CTRL_S *pstCtrl = hTmsgHandle;

    return DLL_COUNT(&pstCtrl->stMsgList);
}


VOID TMSG_StopAll(IN TMSG_HANDLE hTmsgHandle)
{
    TMSG_NODE_S *pstNode, *pstNodeNext;
    TMSG_CTRL_S *pstCtrl = hTmsgHandle;

    DLL_SAFE_SCAN(&pstCtrl->stMsgList, pstNode, pstNodeNext)
    {
        VCLOCK_DestroyTimer(pstCtrl->hVclock, pstNode->hVclockId);
        pstNode->stOpt.pfEventFunc(TMSG_EVENT_FAILED, &pstNode->stOpt, &pstNode->stUserHandle);
        tmsg_FreeNode(pstCtrl, pstNode);
    }
}

VOID TMSG_TickStep(IN TMSG_HANDLE hTmsgHandle)
{
    TMSG_CTRL_S *pstCtrl = hTmsgHandle;

    VCLOCK_Step(pstCtrl->hVclock);
}

static BOOL_T tmsg_RecvedAck(IN TMSG_CTRL_S *pstCtrl, IN TMSG_S *pstTmsg)
{
    TMSG_NODE_S *pstNode, *pstNodeTmp;

    DLL_SAFE_SCAN(&pstCtrl->stMsgList, pstNode, pstNodeTmp)
    {
        if (pstNode->stOpt.stTmsg.uiSequence == pstTmsg->uiSequence)
        {
            pstNode->stOpt.pfEventFunc(TMSG_EVENT_SUCCESS, &pstNode->stOpt, &pstNode->stUserHandle);
            VCLOCK_DestroyTimer(pstCtrl->hVclock, pstNode->hVclockId);
            tmsg_FreeNode(pstCtrl, pstNode);
            return TRUE;
        }
    }

    return FALSE;
}


BOOL_T TMSG_RecvedAck(IN TMSG_HANDLE hTmsgHandle, IN TMSG_S *pstTmsg)
{
    TMSG_CTRL_S *pstCtrl = hTmsgHandle;

    if (pstTmsg->uiFlag & TMSG_FLAG_IS_ACK)
    {
        return tmsg_RecvedAck(pstCtrl, pstTmsg);
    }

    return FALSE;
}



