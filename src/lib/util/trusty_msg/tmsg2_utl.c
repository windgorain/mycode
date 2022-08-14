/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-5-27
* Description: trusty msg 版本2,  基于TMSG实现更高级的功能.
* History:     
* 
* 它和周边模块的关系为:
                 APP
                  |
                TMSG2
                  |
         Packet Process Module
******************************************************************************/

#include "bs.h"

#include "utl/tmsg_utl.h"
#include "utl/tmsg2_utl.h"

#define TMSG2_DFT_RETRY_TIMES 5

typedef struct
{
    TMSG_HANDLE hTmsg;
    PF_TMSG2_SEND_FUNC pfSendFunc;
    PF_TMSG2_RECV_FUNC pfRecvFunc;
    PF_TMSG2_ERR_FUNC pfErrFunc;
    USER_HANDLE_S stUserHandle;
}TMSG2_CTRL_S;;

static VOID tmsg2_TmsgCallBacks(IN int uiEvent, IN TMSG_OPT_S *pstOpt, IN void *ud)
{
    USER_HANDLE_S *pstUserHandle = ud;
    TMSG2_CTRL_S *pstCtrl = pstUserHandle->ahUserHandle[0];
    MBUF_S *pstMbuf = pstUserHandle->ahUserHandle[1];
    TMSG_S *pstTmsg;

    switch (uiEvent)
    {
        case TMSG_EVENT_SEND:
        {
            pstTmsg = MBUF_MTOD(pstMbuf);
            MEM_Copy(pstTmsg, &pstOpt->stTmsg, sizeof(TMSG_S));
            pstTmsg->uiFlag = htonl(pstTmsg->uiFlag);

            pstCtrl->pfSendFunc(pstMbuf, &pstCtrl->stUserHandle);

            break;
        }

        case TMSG_EVENT_SUCCESS:
        {
            MBUF_Free(pstMbuf);
            break;
        }

        case TMSG_EVENT_FAILED:
        {
            pstCtrl->pfErrFunc(0, &pstCtrl->stUserHandle);
            break;
        }

        default:
        {
            break;
        }
    }
}

TMSG2_HANDLE TMSG2_Create
(
    IN PF_TMSG2_SEND_FUNC pfSendFunc,
    IN PF_TMSG2_RECV_FUNC pfRecvFunc,
    IN PF_TMSG2_ERR_FUNC pfErrFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    TMSG2_CTRL_S *pstCtrl;
    TMSG_HANDLE hTmsg;

    hTmsg = TMSG_Create();
    if (NULL == hTmsg)
    {
        return NULL;
    }

    pstCtrl = MEM_ZMalloc(sizeof(TMSG2_CTRL_S));
    if (NULL == pstCtrl)
    {
        TMSG_Destory(hTmsg);
        return NULL;
    }

    pstCtrl->hTmsg = hTmsg;
    pstCtrl->pfSendFunc = pfSendFunc;
    pstCtrl->pfRecvFunc = pfRecvFunc;
    pstCtrl->pfErrFunc = pfErrFunc;

    if (NULL != pstUserHandle)
    {
        pstCtrl->stUserHandle = *pstUserHandle;
    }

    return pstCtrl;
}

VOID TMSG2_Destory(IN TMSG2_HANDLE hTmsg2)
{
    TMSG2_CTRL_S *pstCtrl = hTmsg2;

    if (NULL == pstCtrl)
    {
        return;
    }

    TMSG_Destory(pstCtrl->hTmsg);
    MEM_Free(pstCtrl);
}

BS_STATUS TMSG2_Send(IN TMSG2_HANDLE hTmsg2, IN MBUF_S *pstMbuf)
{
    TMSG2_CTRL_S *pstCtrl = hTmsg2;
    TMSG_OPT_S stOpt;
    USER_HANDLE_S stUserHandle;
    BS_STATUS eRet;

    if (BS_OK != MBUF_Prepend(pstMbuf, sizeof(TMSG_S)))
    {
        MBUF_Free(pstMbuf);
        return BS_NO_MEMORY;
    }

    if (BS_OK != MBUF_MakeContinue(pstMbuf, sizeof(TMSG_S)))
    {
        MBUF_Free(pstMbuf);
        return BS_NO_MEMORY;
    }

    memset(&stOpt, 0, sizeof(stOpt));
    stOpt.uiTicks = 1;
    stOpt.uiRetryTimes = TMSG2_DFT_RETRY_TIMES;
    stOpt.stTmsg.uiFlag = TMSG_FLAG_NEED_ACK;
    stOpt.pfEventFunc = (void*)tmsg2_TmsgCallBacks;

    stUserHandle.ahUserHandle[0] = pstCtrl;
    stUserHandle.ahUserHandle[1] = pstMbuf;

    eRet = TMSG_Send(pstCtrl->hTmsg, &stOpt, &stUserHandle);
    if (eRet != BS_OK)
    {
        MBUF_Free(pstMbuf);
    }

    return eRet;
}


BS_STATUS TMSG2_Input(IN TMSG2_HANDLE hTmsg2, IN MBUF_S *pstMbuf)
{
    TMSG2_CTRL_S *pstCtrl = hTmsg2;
    TMSG_S *pstTmsg;
    TMSG_S stTmsg;
    MBUF_S *pstAck;

    if (BS_OK != MBUF_MakeContinue(pstMbuf, sizeof(TMSG_S)))
    {
        MBUF_Free(pstMbuf);
        return BS_NO_MEMORY;
    }

    pstTmsg = MBUF_MTOD(pstMbuf);

    MEM_Copy(&stTmsg, pstTmsg, sizeof(TMSG_S));
    stTmsg.uiFlag = ntohl(stTmsg.uiFlag);

    if (stTmsg.uiFlag & TMSG_FLAG_IS_ACK)
    {
        if (TMSG_RecvedAck(pstCtrl->hTmsg, &stTmsg) == FALSE)
        {
            /* 超时了 */
            MBUF_Free(pstMbuf);
            return BS_TIME_OUT;
        }
    }
    else if (stTmsg.uiFlag & TMSG_FLAG_NEED_ACK)
    {
        stTmsg.uiFlag = htonl(TMSG_FLAG_IS_ACK);
        pstAck = MBUF_CreateByCopyBuf(200, (UCHAR*)&stTmsg, sizeof(TMSG_S), 0);
        if (NULL != pstAck)
        {
            pstCtrl->pfSendFunc(pstAck, &pstCtrl->stUserHandle);
        }
    }

    MBUF_CutHead(pstMbuf, sizeof(TMSG_S));

    return pstCtrl->pfRecvFunc(pstMbuf, &pstCtrl->stUserHandle);
}

VOID TMSG2_TickStep(IN TMSG2_HANDLE hTmsg2)
{
    TMSG2_CTRL_S *pstCtrl = hTmsg2;

    TMSG_TickStep(pstCtrl->hTmsg);
}


