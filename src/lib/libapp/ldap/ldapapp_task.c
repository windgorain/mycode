/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2017-1-9
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/nap_utl.h"
#include "utl/mypoll_utl.h"
#include "utl/msgque_utl.h"
#include "utl/ldap_utl.h"
#include "comp/comp_ldap.h"

#define LDAPAPP_EVENT_AUTH_MSG 0x1

typedef void (*PF_LDAPAPP_Notify_Func)(int code, UINT id);

typedef struct
{
    LDAPUTL_HANDLE hLdap;
    PF_LDAPAPP_Notify_Func pfNotify;
}LDAPAPP_AUTH_NODE_S;

static MSGQUE_HANDLE g_hLdapAppMsgQue = NULL;
static MYPOLL_HANDLE g_hLdapAppPoller = NULL;
static NAP_HANDLE g_hLdapAppAuthNap = NULL;

static BS_WALK_RET_E _ldapapp_task_EvNotify(IN INT iSocketId, IN UINT uiEvent, IN USER_HANDLE_S *pstUserHandle)
{
    LDAPAPP_AUTH_NODE_S *pstNode = pstUserHandle->ahUserHandle[0];
    BS_STATUS eRet;
    
    if (uiEvent & MYPOLL_EVENT_ERR)
    {
        MyPoll_Del(g_hLdapAppPoller, iSocketId);
        pstNode->pfNotify(LDAPAPP_EVENT_AUTH_FAILED, (UINT)NAP_GetIDByNode(g_hLdapAppAuthNap, pstNode));
        return BS_WALK_CONTINUE;
    }

    eRet = LDAPUTL_Run(pstNode->hLdap);
    if (BS_OK == eRet)
    {
        MyPoll_Del(g_hLdapAppPoller, iSocketId);
        pstNode->pfNotify(LDAPAPP_EVENT_AUTH_SUCCESS, (UINT)NAP_GetIDByNode(g_hLdapAppAuthNap, pstNode));
    }
    else if (BS_CONTINUE != eRet)
    {
        MyPoll_Del(g_hLdapAppPoller, iSocketId);
        pstNode->pfNotify(LDAPAPP_EVENT_AUTH_FAILED, (UINT)NAP_GetIDByNode(g_hLdapAppAuthNap, pstNode));
        return BS_WALK_CONTINUE;
    }

    return BS_WALK_CONTINUE;
}

static BS_STATUS _ldapapp_task_StartAuth(IN LDAPAPP_AUTH_NODE_S *pstNode)
{
    USER_HANDLE_S stUserHandle;

    LDAPUTL_StartAuth(pstNode->hLdap);

    stUserHandle.ahUserHandle[0] = pstNode;

    MyPoll_SetEvent(g_hLdapAppPoller, LDAPUTL_GetSocketID(pstNode->hLdap), MYPOLL_EVENT_IN, _ldapapp_task_EvNotify, &stUserHandle);

    return BS_OK;
}

static VOID _ldapapp_task_ProcessAuthMsg(MSGQUE_MSG_S *pstMsg)
{
    LDAPAPP_AUTH_NODE_S *pstAuthNode;
    UINT uiAuthNodeID = HANDLE_UINT(pstMsg->ahMsg[0]);

    pstAuthNode = NAP_GetNodeByID(g_hLdapAppAuthNap, uiAuthNodeID);
    if (NULL != pstAuthNode)
    {
        _ldapapp_task_StartAuth(pstAuthNode);
    }
}

static VOID _ldapapp_task_ProcessAuthMsgQue()
{
    MSGQUE_MSG_S stMsg;
    
    while (BS_OK == MSGQUE_ReadMsg(g_hLdapAppMsgQue, &stMsg))
    {
        _ldapapp_task_ProcessAuthMsg(&stMsg);
    }
}

static BS_WALK_RET_E _ldapapp_task_UserEvent(IN UINT uiTriggerEvent, IN USER_HANDLE_S *pstUserHandle)
{
    if (uiTriggerEvent & LDAPAPP_EVENT_AUTH_MSG)
    {
        _ldapapp_task_ProcessAuthMsgQue();
    }

    return BS_WALK_CONTINUE;
}

static void _ldapapp_task_main(IN USER_HANDLE_S *pstUserHandle)
{
    while(1)
    {
        MyPoll_Run(g_hLdapAppPoller);
    }
}

BS_STATUS Ldapapp_Task_Init()
{
    g_hLdapAppPoller = MyPoll_Create();
    if (NULL == g_hLdapAppPoller)
    {
        return BS_ERR;
    }

    MyPoll_SetUserEventProcessor(g_hLdapAppPoller, _ldapapp_task_UserEvent, NULL);

    g_hLdapAppMsgQue = MSGQUE_Create(128);
    if (NULL == g_hLdapAppMsgQue)
    {
        MyPoll_Destory(g_hLdapAppPoller);
        g_hLdapAppPoller = NULL;
        return BS_ERR;
    }
    
    if (THREAD_ID_INVALID == 
            THREAD_Create("Ldapapp", NULL, _ldapapp_task_main, NULL))
    {
        MSGQUE_Delete(g_hLdapAppMsgQue);
        g_hLdapAppMsgQue = NULL;
        MyPoll_Destory(g_hLdapAppPoller);
        g_hLdapAppPoller = NULL;
        return BS_ERR;
    }

    return BS_OK;
}



