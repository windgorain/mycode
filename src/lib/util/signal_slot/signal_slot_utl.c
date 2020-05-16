/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-3-9
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/signal_slot_utl.h"
typedef struct
{
    DLL_NODE_S stLinkNode;

    LSTR_S stSender;
    LSTR_S stSignal;
    LSTR_S stReceiver;
    PF_SIG_SLOT_RECEIVER_FUNC pfFunc;
    USER_HANDLE_S stSlotUserHandle;
}_SIGSLOT_SLOT_NODE_S;

typedef struct
{
    DLL_HEAD_S stSlotList;
}_SIGSLOT_HEAD_S;


static INT sigslot_CmpSignal(IN _SIGSLOT_SLOT_NODE_S *pstSlotNode, IN LSTR_S *pstSender, IN LSTR_S *pstSignal)
{
    INT iCmpRet;

    if (pstSlotNode->stSignal.uiLen != pstSignal->uiLen)
    {
        return -1;
    }

    if ((pstSlotNode->stSender.uiLen != 0) && (pstSlotNode->stSender.uiLen != pstSender->uiLen))
    {
        return -1;
    }

    if (pstSlotNode->stSender.uiLen != 0)
    {
        iCmpRet = strcmp(pstSlotNode->stSender.pcData, pstSender->pcData);
        if (0 != iCmpRet)
        {
            return iCmpRet;
        }
    }

    iCmpRet = strcmp(pstSlotNode->stSignal.pcData, pstSignal->pcData);
    if (0 != iCmpRet)
    {
        return iCmpRet;
    }

    return 0;
}

static INT sigslot_CmpSlot
(
    IN _SIGSLOT_SLOT_NODE_S *pstSlotNode,
    IN LSTR_S *pstSender,
    IN LSTR_S *pstSignal,
    IN LSTR_S *pstReceiver,
    IN PF_SIG_SLOT_RECEIVER_FUNC pfSlotFunc
)
{
    INT iCmpRet;

    if ((pstSlotNode->stSender.uiLen != pstSender->uiLen)
        || (pstSlotNode->stSignal.uiLen != pstSignal->uiLen)
        || (pstSlotNode->stReceiver.uiLen != pstReceiver->uiLen)
        || (pstSlotNode->pfFunc != pfSlotFunc))
    {
        return -1;
    }

    if (pstSender->uiLen != 0)
    {
        iCmpRet = strcmp(pstSlotNode->stSender.pcData, pstSender->pcData);
        if (0 != iCmpRet)
        {
            return iCmpRet;
        }
    }

    iCmpRet = strcmp(pstSlotNode->stSignal.pcData, pstSignal->pcData);
    if (0 != iCmpRet)
    {
        return iCmpRet;
    }

    iCmpRet = strcmp(pstSlotNode->stReceiver.pcData, pstReceiver->pcData);
    if (0 != iCmpRet)
    {
        return iCmpRet;
    }

    return 0;
}

SIG_SLOT_HANDLE SigSlot_Create()
{
    _SIGSLOT_HEAD_S *pstSSHead;

    pstSSHead = MEM_ZMalloc(sizeof(_SIGSLOT_HEAD_S));
    if (NULL == pstSSHead)
    {
        return NULL;
    }

    DLL_INIT(&pstSSHead->stSlotList);

    return pstSSHead;
}

BS_STATUS SigSlot_Connect
(
    IN SIG_SLOT_HANDLE hSigSlot,
    IN CHAR *pcSender,
    IN CHAR *pcSignal,
    IN CHAR *pcReceiver,
    IN PF_SIG_SLOT_RECEIVER_FUNC pfSlotFunc,
    IN USER_HANDLE_S *pstSlotUserHandle
)
{
    _SIGSLOT_HEAD_S *pstSSHead = hSigSlot;
    _SIGSLOT_SLOT_NODE_S *pstSlotNode;

    if (NULL == pstSSHead)
    {
        return BS_NULL_PARA;
    }

    pstSlotNode = MEM_ZMalloc(sizeof(_SIGSLOT_SLOT_NODE_S));
    if (NULL == pstSlotNode)
    {
        return BS_NO_MEMORY;
    }

    if (NULL != pcSender)
    {
        pstSlotNode->stSender.pcData = pcSender;
        pstSlotNode->stSender.uiLen = strlen(pcSender);
    }
    pstSlotNode->stSignal.pcData = pcSignal;
    pstSlotNode->stSignal.uiLen = strlen(pcSignal);
    pstSlotNode->stReceiver.pcData = pcReceiver;
    pstSlotNode->stReceiver.uiLen = strlen(pcReceiver);
    pstSlotNode->pfFunc = pfSlotFunc;
    if (NULL != pstSlotUserHandle)
    {
        pstSlotNode->stSlotUserHandle = *pstSlotUserHandle;
    }

    DLL_ADD_TO_HEAD(&pstSSHead->stSlotList, pstSlotNode);

    return BS_OK;
}

VOID SigSlot_DisConnect
(
    IN SIG_SLOT_HANDLE hSigSlot,
    IN CHAR *pcSender,
    IN CHAR *pcSignal,
    IN CHAR *pcReceiver,
    IN PF_SIG_SLOT_RECEIVER_FUNC pfSlotFunc
)
{
    _SIGSLOT_HEAD_S *pstSSHead = hSigSlot;
    _SIGSLOT_SLOT_NODE_S *pstSlotNode;
    _SIGSLOT_SLOT_NODE_S *pstSlotNodeTmp;
    LSTR_S stSender;
    LSTR_S stSignal;
    LSTR_S stReceiver;

    if (NULL == pstSSHead)
    {
        return;
    }

    stSender.pcData = pcSender;
    stSender.uiLen = 0;
    if (NULL != pcSender)
    {
        stSender.uiLen = strlen(pcSender);
    }

    stSignal.pcData = pcSignal;
    stSignal.uiLen = strlen(pcSignal);
    stReceiver.pcData = pcReceiver;
    stReceiver.uiLen = strlen(pcReceiver);

    DLL_SAFE_SCAN(&pstSSHead->stSlotList, pstSlotNode, pstSlotNodeTmp)
    {
        if (0 == sigslot_CmpSlot(pstSlotNode, &stSender, &stSignal, &stReceiver, pfSlotFunc))
        {
            DLL_DEL(&pstSSHead->stSlotList, pstSlotNode);
            MEM_Free(pstSlotNode);
        }
    }

    return;
}

BS_STATUS SigSlot_SendSignal
(
    IN SIG_SLOT_HANDLE hSigSlot,
    IN CHAR *pcSender,
    IN CHAR *pcSignal,
    IN SIG_SLOT_PARAM_S *pstParam
)
{
    _SIGSLOT_HEAD_S *pstSSHead = hSigSlot;
    _SIGSLOT_SLOT_NODE_S *pstSlotNode;
    LSTR_S stSender;
    LSTR_S stSignal;

    if (NULL == pstSSHead)
    {
        return BS_NULL_PARA;
    }

    stSender.pcData = pcSender;
    stSender.uiLen = strlen(pcSender);
    stSignal.pcData = pcSignal;
    stSignal.uiLen = strlen(pcSignal);

    DLL_SCAN(&pstSSHead->stSlotList, pstSlotNode)
    {
        if (0 == sigslot_CmpSignal(pstSlotNode, &stSender, &stSignal))
        {
            pstSlotNode->pfFunc(pcSender, pcSignal, pstParam, &pstSlotNode->stSlotUserHandle);
        }
    }

    return BS_OK;
}

