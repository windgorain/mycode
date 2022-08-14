/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2013-4-2
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/msgque_utl.h"
#include "utl/event_utl.h"
#include "comp/comp_if.h"

#include "app/wan_pub.h"
#include "app/if_pub.h"

#include "../h/wan_ifnet.h"
#include "../h/wan_ipfwd.h"

typedef struct
{
    THREAD_ID uiTID;
    UINT uiIfIndex;
    MSGQUE_HANDLE hMsgQue;
    EVENT_HANDLE hEvent;
}WAN_INLOOP_CTRL_S;

static WAN_INLOOP_CTRL_S g_stWanInLoop;

static VOID wan_inloop_Recv(IN UINT uiIfIndex, IN MBUF_S *pstMbuf)
{
    WAN_IpFwd_Input(pstMbuf);
}

static VOID _wan_inloop_Recv(IN UINT uiIfIndex, IN MBUF_S *pstMbuf, IN USHORT usProtoType)
{
    wan_inloop_Recv(uiIfIndex, pstMbuf);
}

static void _wan_inloop_Main(IN USER_HANDLE_S *pstUserHandle)
{
    MSGQUE_MSG_S stMsg;
    UINT uiIfIndex;
    UINT64 event;
    USHORT usProtoType;
    MBUF_S *pstMbuf;
    
    while (1)
    {
        Event_Read(g_stWanInLoop.hEvent, 0xffffffff, &event,
                EVENT_FLAG_WAIT, BS_WAIT_FOREVER);
        if (BS_OK == MSGQUE_ReadMsg(g_stWanInLoop.hMsgQue, &stMsg))
        {
            uiIfIndex = HANDLE_UINT(stMsg.ahMsg[0]);
            usProtoType = HANDLE_UINT(stMsg.ahMsg[1]);
            pstMbuf = stMsg.ahMsg[2];
            
            _wan_inloop_Recv(uiIfIndex, pstMbuf, usProtoType);
        }
    }
}

BS_STATUS WAN_InLoop_LinkOutput (IN UINT uiIfIndex, IN MBUF_S *pstMbuf, IN USHORT usProtoType)
{
    MSGQUE_MSG_S stMsg;

    if (uiIfIndex != g_stWanInLoop.uiIfIndex)
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    stMsg.ahMsg[0] = UINT_HANDLE(uiIfIndex);
    stMsg.ahMsg[1] = UINT_HANDLE(usProtoType);
    stMsg.ahMsg[2] = pstMbuf;

    if (BS_OK != MSGQUE_WriteMsg(g_stWanInLoop.hMsgQue, &stMsg))
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    Event_Write(g_stWanInLoop.hEvent, 1);

    return BS_OK;
}

static void _wan_inloop_Clean()
{
    if (g_stWanInLoop.hEvent) {
        Event_Delete(g_stWanInLoop.hEvent);
        g_stWanInLoop.hEvent = NULL;
    }

    if (g_stWanInLoop.hMsgQue) {
        MSGQUE_Delete(g_stWanInLoop.hMsgQue);
        g_stWanInLoop.hMsgQue = NULL;
    }

    if (g_stWanInLoop.uiIfIndex) {
        IFNET_DeleteIf(g_stWanInLoop.uiIfIndex);
        g_stWanInLoop.uiIfIndex = 0;
    }

}

BS_STATUS _wan_inloop_Init()
{
    g_stWanInLoop.hEvent = Event_Create();
    if (NULL == g_stWanInLoop.hEvent) {
        _wan_inloop_Clean();
        RETURN(BS_ERR);
    }
    g_stWanInLoop.hMsgQue = MSGQUE_Create(128);
    if (NULL == g_stWanInLoop.hMsgQue) {
        _wan_inloop_Clean();
        RETURN(BS_ERR);
    }

    g_stWanInLoop.uiIfIndex = IFNET_CreateIf(IF_INLOOP_IF_TYPE_MAME);
    if (0 == g_stWanInLoop.uiIfIndex) {
        _wan_inloop_Clean();
        RETURN(BS_ERR);
    }

    g_stWanInLoop.uiTID = THREAD_Create("inloop", NULL, _wan_inloop_Main, NULL);
    if (g_stWanInLoop.uiTID == THREAD_ID_INVALID) {
        _wan_inloop_Clean();
        RETURN(BS_ERR);
    }

    return BS_OK;
}

BS_STATUS WAN_InLoop_Init()
{
    return _wan_inloop_Init();
}

UINT WAN_InLoop_GetIfIndex()
{
    return g_stWanInLoop.uiIfIndex;
}

