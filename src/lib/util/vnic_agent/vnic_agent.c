/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-8-27
* Description: 
* History:     
******************************************************************************/
/* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_VNICAGENT


#include "bs.h"

#include "utl/vnic_agent.h"
#include "utl/vnic_lib.h"
#include "utl/msgque_utl.h"

#define _VNIC_AGENT_QUIT_EVENT          0x1
#define _VNIC_AGENT_SEND_DATA_EVENT     0x2  /* 发送数据 */

/* 消息类型 */
#define _VNIC_AGENT_WRITE_MSG   1

#define _VNIC_AGENT_RESERVED_MBUF_HEAD_SPACE 300

typedef struct
{
    THREAD_ID ulVnicAgentReaderTID;
    THREAD_ID ulVnicAgentWriterTID;
    EVENT_HANDLE hReaderEventId;
    EVENT_HANDLE hWriterEventId;
    MSGQUE_HANDLE hWriterMsgque;
    VNIC_HANDLE hVnic;
    HANDLE hUserHandle;
}_VNIC_AGENT_CTRL_S;

static VOID _vnic_agent_ProcessReadedData
(
    IN _VNIC_AGENT_CTRL_S *pstVnicAgentCtrl,
    IN MBUF_CLUSTER_S *pstCluster,
    IN UINT uiDataLen,
    IN VNIC_AGENT_CB_FUNC pfCallBackFunc,
    IN HANDLE hUserHandle
)
{
    MBUF_S *pstMbuf;
    
    if (uiDataLen == 0)
    {
        MBUF_FreeCluster (pstCluster);
        return;
    }
        
    pstMbuf = MBUF_CreateByCluster (pstCluster,
        _VNIC_AGENT_RESERVED_MBUF_HEAD_SPACE, uiDataLen, MBUF_DATA_DATA);

    if (NULL == pstMbuf)
    {
        MBUF_FreeCluster (pstCluster);
        return;
    }

    pfCallBackFunc (pstVnicAgentCtrl, pstMbuf, hUserHandle);

    return;
}

static void _vnic_agent_ReaderMain(IN USER_HANDLE_S *pstUserHandle)
{
    VNIC_HANDLE hVnic = pstUserHandle->ahUserHandle[0];
    _VNIC_AGENT_CTRL_S *pstVnicAgentCtrl = pstUserHandle->ahUserHandle[1];
    VNIC_AGENT_CB_FUNC pfCallBackFunc = (VNIC_AGENT_CB_FUNC) pstUserHandle->ahUserHandle[2];
    HANDLE hUserHandle = pstUserHandle->ahUserHandle[3];
    MBUF_CLUSTER_S *pstCluster = NULL;
    UINT ulReadLen;
    UINT64 uiEvent;
    UINT ulMode = 0;
    UINT ulRet;

    while (1)
    {
        Event_Read (pstVnicAgentCtrl->hReaderEventId, _VNIC_AGENT_QUIT_EVENT, &uiEvent, ulMode, 200);
        if (uiEvent & _VNIC_AGENT_QUIT_EVENT)
        {
            break;
        }

        ulMode = 0;

        pstCluster = MBUF_CreateCluster();
        if (NULL == pstCluster)
        {
            break;
        }
        
        ulRet = VNIC_Read (hVnic, BS_WAIT, BS_WAIT_FOREVER,
            pstCluster->pucData + _VNIC_AGENT_RESERVED_MBUF_HEAD_SPACE,
            MBUF_CLUSTER_SIZE (pstCluster) - _VNIC_AGENT_RESERVED_MBUF_HEAD_SPACE, &ulReadLen);

        if (BS_OK == ulRet)
        {
            _vnic_agent_ProcessReadedData(pstVnicAgentCtrl, pstCluster, ulReadLen, pfCallBackFunc, hUserHandle);
        }
        else 
        {
            MBUF_FreeCluster (pstCluster);
            if ((BS_TIME_OUT != ulRet) && (BS_NOT_COMPLETE != RETCODE(ulRet)))
            {
                ulMode = EVENT_FLAG_WAIT;
            }
        }
    }

    pstVnicAgentCtrl->ulVnicAgentReaderTID = 0;
    
    return;
}

static inline BS_STATUS _vnic_agent_SendData (IN VNIC_HANDLE hVnic, IN MBUF_S *pstMbuf)
{
    UCHAR *pucData;
    UINT ulWriteLen;
    
    if (BS_OK != MBUF_MakeContinue(pstMbuf, MBUF_TOTAL_DATA_LEN(pstMbuf)))
    {
        BS_WARNNING (("Mbuf is too long, size=%d", MBUF_TOTAL_DATA_LEN(pstMbuf)));
        MBUF_Free (pstMbuf);
        return (BS_NOT_SUPPORT);
    }

    pucData = MBUF_MTOD (pstMbuf);

    VNIC_Write (hVnic, pucData, MBUF_TOTAL_DATA_LEN(pstMbuf), &ulWriteLen);

    MBUF_Free (pstMbuf);

    return BS_OK;
}

static inline BS_STATUS _vnic_agent_DealSendDataMsg (IN VNIC_HANDLE hVnic, IN MSGQUE_MSG_S *pstMsg)
{
    MBUF_S *pstMbuf;

    pstMbuf = (MBUF_S*)pstMsg->ahMsg[1];

    _vnic_agent_SendData (hVnic, pstMbuf);

    return BS_OK;
}

static inline BS_STATUS _vnic_agent_DealWriteMsg (IN VNIC_HANDLE hVnic, IN MSGQUE_MSG_S *pstMsg)
{
    UINT ulMsgType;
    BS_STATUS eRet;

    ulMsgType = HANDLE_UINT(pstMsg->ahMsg[0]);

    switch (ulMsgType)
    {
        case _VNIC_AGENT_WRITE_MSG:
            eRet = _vnic_agent_DealSendDataMsg(hVnic, pstMsg);
            break;

        default:
            eRet = BS_NOT_SUPPORT;
            BS_WARNNING(("Not support yet!"));
            break;
    }

    return eRet;
}

static inline BS_STATUS _vnic_agent_DropWriteMsg (IN VNIC_HANDLE hVnic, IN MSGQUE_MSG_S *pstMsg)
{
    UINT ulMsgType;
    MBUF_S *pstMbuf;

    ulMsgType = HANDLE_UINT(pstMsg->ahMsg[0]);

    switch (ulMsgType)
    {
        case _VNIC_AGENT_WRITE_MSG:
            pstMbuf = (MBUF_S*)pstMsg->ahMsg[1];
            MBUF_Free(pstMbuf);
            break;

        default:
            BS_WARNNING(("Not support yet!"));
            break;
    }

    return BS_OK;
}

static void _vnic_agent_WriterMain(IN USER_HANDLE_S *pstUserHandle)
{
    UINT64 uiEvent;
    MSGQUE_MSG_S stMsg;
    VNIC_HANDLE hVnic = pstUserHandle->ahUserHandle[0];
    _VNIC_AGENT_CTRL_S *pstVnicAgentCtrl = pstUserHandle->ahUserHandle[1];
    
    for (;;)
    {
        Event_Read (pstVnicAgentCtrl->hWriterEventId,
            _VNIC_AGENT_SEND_DATA_EVENT | _VNIC_AGENT_QUIT_EVENT,
            &uiEvent, EVENT_FLAG_WAIT, BS_WAIT_FOREVER);

        if (uiEvent & _VNIC_AGENT_QUIT_EVENT)
        {
            break;
        }

        if (uiEvent & _VNIC_AGENT_SEND_DATA_EVENT)
        {
            while (BS_OK == MSGQUE_ReadMsg(pstVnicAgentCtrl->hWriterMsgque, &stMsg))
            {
                _vnic_agent_DealWriteMsg (hVnic, &stMsg);
            }
        }
    }

    while (BS_OK == MSGQUE_ReadMsg (pstVnicAgentCtrl->hWriterMsgque, &stMsg))
    {
        _vnic_agent_DropWriteMsg (hVnic, &stMsg);
    }

    pstVnicAgentCtrl->ulVnicAgentWriterTID = 0;
    
    return;
}

VNIC_AGENT_HANDLE VNIC_Agent_Create()
{
    _VNIC_AGENT_CTRL_S *pstVnicAgentCtrl;

    pstVnicAgentCtrl = MEM_ZMalloc (sizeof(_VNIC_AGENT_CTRL_S));
    if (NULL == pstVnicAgentCtrl)
    {
        return NULL;
    }

    if (NULL == (pstVnicAgentCtrl->hReaderEventId = Event_Create()))
    {
        VNIC_Agent_Close(pstVnicAgentCtrl);
        return NULL;
    }

    if (NULL == (pstVnicAgentCtrl->hWriterEventId = Event_Create()))
    {
        VNIC_Agent_Close(pstVnicAgentCtrl);
        return NULL;
    }

    if (NULL == (pstVnicAgentCtrl->hWriterMsgque = MSGQUE_Create(128)))
    {
        VNIC_Agent_Close(pstVnicAgentCtrl);
        return NULL;
    }

    return pstVnicAgentCtrl;  
}

BS_STATUS VNIC_Agent_Close (IN VNIC_AGENT_HANDLE hVnicAgent)
{
    _VNIC_AGENT_CTRL_S *pstVnicAgentCtrl = (_VNIC_AGENT_CTRL_S *)hVnicAgent;

    if (NULL == pstVnicAgentCtrl)
    {
        BS_DBGASSERT (0);
        RETURN(BS_NULL_PARA);
    }

    VNIC_Agent_Stop(hVnicAgent);

    if (NULL != pstVnicAgentCtrl->hReaderEventId)
    {
        Event_Delete(pstVnicAgentCtrl->hReaderEventId);
    }

    if (NULL != pstVnicAgentCtrl->hWriterEventId)
    {
        Event_Delete(pstVnicAgentCtrl->hWriterEventId);
    }

    if (NULL != pstVnicAgentCtrl->hWriterMsgque)
    {
        MSGQUE_Delete(pstVnicAgentCtrl->hWriterMsgque);
    }

    MEM_Free (pstVnicAgentCtrl);

    return BS_OK;
}

BS_STATUS VNIC_Agent_Start
(
    IN VNIC_AGENT_HANDLE hVnicAgent,
    IN VNIC_AGENT_CB_FUNC pfFunc, 
    IN HANDLE hUserHandle
)
{
    _VNIC_AGENT_CTRL_S *pstVnicAgentCtrl = (_VNIC_AGENT_CTRL_S *)hVnicAgent;
    USER_HANDLE_S stUserHandle;

    if (pstVnicAgentCtrl->ulVnicAgentReaderTID != 0)
    {
        return BS_OK;
    }

    stUserHandle.ahUserHandle[0] = pstVnicAgentCtrl->hVnic;
    stUserHandle.ahUserHandle[1] = pstVnicAgentCtrl;
    stUserHandle.ahUserHandle[2] = pfFunc;
    stUserHandle.ahUserHandle[3] = hUserHandle;

    if (THREAD_ID_INVALID == (pstVnicAgentCtrl->ulVnicAgentReaderTID = 
                THREAD_Create("VnicAgentR", NULL,
                    _vnic_agent_ReaderMain, &stUserHandle))) {
        BS_WARNNING(("Can't create vnic agent thread"));
        RETURN(BS_ERR);
    }

    if (THREAD_ID_INVALID == (pstVnicAgentCtrl->ulVnicAgentWriterTID = 
                THREAD_Create("VnicAgentW", NULL,
                    _vnic_agent_WriterMain, &stUserHandle))) {
        Event_Write (pstVnicAgentCtrl->hReaderEventId, _VNIC_AGENT_QUIT_EVENT);
        VNIC_Signale (pstVnicAgentCtrl->hVnic);
        BS_WARNNING(("Can't create vnic agent thread"));
        RETURN(BS_ERR);
    }

    return BS_OK;
}

VOID VNIC_Agent_Stop(IN VNIC_AGENT_HANDLE hVnicAgent)
{
    _VNIC_AGENT_CTRL_S *pstVnicAgentCtrl = (_VNIC_AGENT_CTRL_S *)hVnicAgent;

    if (pstVnicAgentCtrl->ulVnicAgentReaderTID != 0)
    {
        if (NULL != pstVnicAgentCtrl->hReaderEventId)
        {
            Event_Write (pstVnicAgentCtrl->hReaderEventId, _VNIC_AGENT_QUIT_EVENT);
        }

        if (NULL != pstVnicAgentCtrl->hWriterEventId)
        {
            Event_Write (pstVnicAgentCtrl->hWriterEventId, _VNIC_AGENT_QUIT_EVENT);
        }

        if (NULL != pstVnicAgentCtrl->hVnic)
        {
            VNIC_Signale (pstVnicAgentCtrl->hVnic);
        }
    }

    /* 等待目标线程退出 */
    while ((pstVnicAgentCtrl->ulVnicAgentReaderTID != 0)
        || (pstVnicAgentCtrl->ulVnicAgentWriterTID != 0))
    {
        Sleep(10);
    }
}

BS_STATUS VNIC_Agent_Write(IN VNIC_AGENT_HANDLE hVnicAgent, IN MBUF_S *pstMbuf)
{
    MSGQUE_MSG_S stMsg;
    _VNIC_AGENT_CTRL_S *pstVnicAgentCtrl = (_VNIC_AGENT_CTRL_S *)hVnicAgent;

    stMsg.ahMsg[0] = UINT_HANDLE(_VNIC_AGENT_WRITE_MSG);
    stMsg.ahMsg[1] = pstMbuf;

    if (BS_OK != MSGQUE_WriteMsg(pstVnicAgentCtrl->hWriterMsgque, &stMsg)) {
        MBUF_Free (pstMbuf);
        return BS_FULL;
    }

    Event_Write(pstVnicAgentCtrl->hWriterEventId, _VNIC_AGENT_SEND_DATA_EVENT);

    return BS_OK;
}

BS_STATUS VNIC_Agent_WriteData(IN VNIC_AGENT_HANDLE hVnicAgent, IN UCHAR *pucData, IN UINT uiDataLen)
{
    MBUF_S *pstMbuf;

    pstMbuf = MBUF_CreateByCopyBuf(0, pucData, uiDataLen, MBUF_DATA_DATA);
    if (NULL == pstMbuf)
    {
        return BS_NO_MEMORY;
    }

    return VNIC_Agent_Write(hVnicAgent, pstMbuf);
}

HANDLE VNIC_Agent_GetUserData (IN VNIC_AGENT_HANDLE hVnicAgent)
{
    _VNIC_AGENT_CTRL_S *pstVnicAgentCtrl = (_VNIC_AGENT_CTRL_S *)hVnicAgent;

    if (NULL == pstVnicAgentCtrl)
    {
        BS_DBGASSERT (0);
        return 0;
    }

    return pstVnicAgentCtrl->hUserHandle;
}

BS_STATUS VNIC_Agent_SetUserData (IN VNIC_AGENT_HANDLE hVnicAgent, IN HANDLE hUserHandle)
{
    _VNIC_AGENT_CTRL_S *pstVnicAgentCtrl = (_VNIC_AGENT_CTRL_S *)hVnicAgent;

    if (NULL == pstVnicAgentCtrl)
    {
        BS_DBGASSERT (0);
        RETURN(BS_NULL_PARA);
    }

    pstVnicAgentCtrl->hUserHandle = hUserHandle;

    return BS_OK;
}

VOID VNIC_Agent_SetVnic(IN VNIC_AGENT_HANDLE hVnicAgent, IN VNIC_HANDLE hVnic)
{
    _VNIC_AGENT_CTRL_S *pstVnicAgentCtrl = (_VNIC_AGENT_CTRL_S *)hVnicAgent;

    if (NULL != pstVnicAgentCtrl)
    {
        pstVnicAgentCtrl->hVnic = hVnic;
    }
}

VNIC_HANDLE VNIC_Agent_GetVnic(IN VNIC_AGENT_HANDLE hVnicAgent)
{
    _VNIC_AGENT_CTRL_S *pstVnicAgentCtrl = (_VNIC_AGENT_CTRL_S *)hVnicAgent;

    if (NULL == pstVnicAgentCtrl)
    {
        return 0;
    }

    return pstVnicAgentCtrl->hVnic;    
}

