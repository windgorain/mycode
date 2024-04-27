/******************************************************************************
* Copyright (C),    LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-6-8
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mutex_utl.h"
#include "comp/comp_if.h"

#include "../../inc/vnet_node.h"

#include "../inc/vnetc_master.h"
#include "../inc/vnetc_node.h"
#include "../inc/vnetc_phy.h"
#include "../inc/vnetc_ses.h"


#define VNETC_NID_BUCKET_NUM 1024

#define _VNETC_NID_OLD_TIME (1000 * 3600) 

enum
{
    VNETC_NID_PRI_SET = 0,  
    VNETC_NID_PRI_DIRETECT,
    VNETC_NID_PRI_LEARN
};

static HASH_S * g_hVnetcNidHandle;
static MUTEX_S g_stVnetcNodeMutex;    

static UINT vnetc_node_HashIndex(IN VOID *pstHashNode)
{
    VNETC_NID_S * pstNidNode = pstHashNode;

    return pstNidNode->uiNID;
}

static INT  vnetc_node_Cmp(IN VOID * pstHashNode, IN VOID * pstNodeToFind)
{
    VNETC_NID_S *pstNode1 = pstHashNode;
    VNETC_NID_S *pstNode2 = pstNodeToFind;

    return (INT)(pstNode1->uiNID - pstNode2->uiNID);
}

VNETC_NID_S * vnetc_node_Find(IN UINT uiNID)
{
    VNETC_NID_S stToFind;

    stToFind.uiNID = uiNID;
    
    return (VNETC_NID_S*) HASH_Find(g_hVnetcNidHandle, vnetc_node_Cmp, &stToFind);
}

static CHAR * vnetc_node_GetDirectString(IN UCHAR ucDirectFlag)
{
    CHAR *pcString = "";

    switch (ucDirectFlag)
    {
        case VNETC_NODE_DIRECT_INIT:
        {
            pcString = "Init";
            break;
        }

        case VNETC_NODE_DIRECT_DETECTING:
        {
            pcString = "Detecting";
            break;
        }

        case VNETC_NODE_DIRECT_OK:
        {
            pcString = "OK";
            break;
        }

        case VNETC_NODE_DIRECT_FAILED:
        {
            pcString = "Failed";
            break;
        }

        default:
        {
            break;
        }
    }

    return pcString;
}

static VOID vnetc_node_Del(IN UINT uiNID)
{
    VNETC_NID_S *pstNid;

    pstNid = vnetc_node_Find(uiNID);

    if (NULL != pstNid)
    {
        if (pstNid->uiSesID != 0)
        {
            VNETC_SES_Close(pstNid->uiSesID);
        }

        HASH_Del(g_hVnetcNidHandle, pstNid);
        MEM_Free(pstNid);
    }

    return;
}

static VOID vnetc_node_SetSesID(VNETC_NID_S *pstNid, IN UINT uiSesID)
{
    if (pstNid->uiSesID == uiSesID)
    {
        return;
    }

    pstNid->uiSesID = uiSesID;

    if (uiSesID != 0)
    {
        VNETC_SES_SetProperty(uiSesID, VNETC_SES_PROPERTY_NODE_ID, UINT_HANDLE(pstNid->uiNID));
        pstNid->ucDirectStatus = VNETC_NODE_DIRECT_OK;
    }
    else
    {
        if (pstNid->ucDirectStatus != VNETC_NODE_DIRECT_FAILED)
        {
            pstNid->ucDirectStatus = VNETC_NODE_DIRECT_INIT;
        }
    }
}

static VOID vnetc_node_LearnSesID(VNETC_NID_S *pstNid, IN UINT uiSesID)
{
    UINT uiNewPri;
    UINT uiOldPri;
    UINT uiOldSes;

    if (uiSesID == 0)
    {
        return;
    }

    if (pstNid->uiSesID == uiSesID)
    {
        return;
    }

    if (pstNid->uiSesID == 0)
    {
        vnetc_node_SetSesID(pstNid, uiSesID);
        return;
    }

    
    uiNewPri = VNETC_NODE_Self();
    uiOldPri = uiNewPri;

    if (VNETC_SES_GetType(uiSesID) == SES_TYPE_APPECTER)
    {
        uiNewPri = pstNid->uiNID;
    }

    if (VNETC_SES_GetType(pstNid->uiSesID) == SES_TYPE_APPECTER)
    {
        uiOldPri = pstNid->uiNID;
    }

    if (uiNewPri <= uiOldPri)
    {
        uiOldSes = pstNid->uiSesID;
        vnetc_node_SetSesID(pstNid, uiSesID);
        VNETC_SES_SetProperty(uiOldSes, VNETC_SES_PROPERTY_NODE_ID, NULL);
        VNETC_SES_Close(uiOldSes);  
    }
    else
    {
        VNETC_SES_Close(uiSesID);  
    }

    return;
}

static VOID vnetc_node_OldTimeOut(IN HANDLE hTimerHandle, IN USER_HANDLE_S *pstUserHandle)
{
    VNETC_NID_S *pstNid = pstUserHandle->ahUserHandle[0];
    
    VNETC_NODE_Del(pstNid->uiNID);
}

static BS_STATUS vnetc_node_StartOldTimer(IN VNETC_NID_S *pstNid)
{
    USER_HANDLE_S stUserHandle;
    
    if (pstNid->hOldTimer != NULL)
    {
        return BS_OK;
    }

    stUserHandle.ahUserHandle[0] = pstNid;

    pstNid->hOldTimer = VNETC_Master_AddTimer(_VNETC_NID_OLD_TIME, TIMER_FLAG_CYCLE, vnetc_node_OldTimeOut, &stUserHandle);
    if (NULL == pstNid->hOldTimer)
    {
        return BS_ERR;
    }

    return BS_OK;
}

static VOID vnetc_node_StopOldTimer(IN VNETC_NID_S *pstNid)
{
    if (pstNid->hOldTimer == NULL)
    {
        return;
    }

    VNETC_Master_DelTimer(pstNid->hOldTimer);
    pstNid->hOldTimer = NULL;
}

static BS_STATUS vnetc_node_Add
(
    IN UINT uiNID,
    IN UINT uiIfIndex,
    IN UINT uiSesID,
    IN UINT uiFlag,
    IN UINT uiPRI
)
{
    VNETC_NID_S *pstNid;

    pstNid = vnetc_node_Find(uiNID);

    if (NULL == pstNid)
    {
        pstNid = MEM_ZMalloc(sizeof(VNETC_NID_S));
        if (NULL == pstNid)
        {
            return BS_NO_MEMORY;
        }

        pstNid->uiNID = uiNID;
        pstNid->uiIfIndex = uiIfIndex;
        vnetc_node_LearnSesID(pstNid, uiSesID);
        pstNid->uiPRI = uiPRI;
        pstNid->uiFlag = uiFlag;

        HASH_Add(g_hVnetcNidHandle, pstNid);
    }
    else
    {
        if (uiPRI <= pstNid->uiPRI)
        {
            pstNid->uiIfIndex = uiIfIndex;
            vnetc_node_LearnSesID(pstNid, uiSesID);
            pstNid->uiPRI = uiPRI;
            pstNid->uiFlag = uiFlag;
            pstNid->uiAge = 0;
        }
    }

    if (pstNid->uiFlag & VNETC_NODE_FLAG_STATIC)
    {
        vnetc_node_StopOldTimer(pstNid);
    }
    else if (pstNid->hOldTimer == NULL)
    {
        vnetc_node_StartOldTimer(pstNid);
    }
    else
    {
        VNETC_Master_RefreshTimer(pstNid->hOldTimer);
    }

    return BS_OK;
}

static VOID vnetc_node_SesCloseEvent(IN UINT uiSesID, IN HANDLE *phPropertys, IN USER_HANDLE_S *pstUserHandle)
{
    UINT uiNodeID;

    uiNodeID = HANDLE_UINT(phPropertys[VNETC_SES_PROPERTY_NODE_ID]);

    if (VNET_NID_TYPE(uiNodeID) != VNET_NID_TYPE_CLIENT)
    {
        return;
    }

    if (uiNodeID != 0)
    {
        VNETC_NODE_SesClosed(uiNodeID, uiSesID);
    }
}

BS_STATUS VNETC_NODE_Init()
{
    g_hVnetcNidHandle = HASH_CreateInstance(VNETC_NID_BUCKET_NUM, vnetc_node_HashIndex);
    if (NULL == g_hVnetcNidHandle)
    {
        return BS_ERR;
    }

    VNETC_SES_RegCloseNotify(vnetc_node_SesCloseEvent, NULL);

    MUTEX_Init(&g_stVnetcNodeMutex);

    VNETC_NODE_Set(VNET_NID_SERVER, 0, 0, VNETC_NODE_FLAG_STATIC);

    return BS_OK;
}

BS_STATUS VNETC_NODE_Set
(
    IN UINT uiNID,
    IN UINT uiIfIndex,
    IN UINT uiSesID,
    IN UINT uiFlag
)
{
    BS_STATUS eRet;
    
    MUTEX_P(&g_stVnetcNodeMutex);
    eRet = vnetc_node_Add(uiNID, uiIfIndex, uiSesID, uiFlag, VNETC_NID_PRI_SET);
    MUTEX_V(&g_stVnetcNodeMutex);

    return eRet;
}

BS_STATUS VNETC_NODE_Learn
(
    IN UINT uiNID,
    IN UINT uiIfIndex,
    IN UINT uiSesID
)
{
    BS_STATUS eRet;
    UINT uiPRI = VNETC_NID_PRI_LEARN;

    if (uiSesID != 0)
    {
        uiPRI = VNETC_NID_PRI_DIRETECT;
    }
    
    MUTEX_P(&g_stVnetcNodeMutex);
    eRet = vnetc_node_Add(uiNID, uiIfIndex, uiSesID, 0, uiPRI);
    MUTEX_V(&g_stVnetcNodeMutex);

    return eRet;
}


static VOID vnetc_node_SesClosed(IN UINT uiNID, IN UINT uiSesID)
{
    VNETC_NID_S *pstNode;
    
    pstNode = VNETC_NODE_GetNode(uiNID);
    if (NULL == pstNode)
    {
        return;
    }

    if (pstNode->uiPRI < VNETC_NID_PRI_DIRETECT)
    {
        return;
    }

    if (pstNode->uiSesID != uiSesID)
    {
        return;
    }

    pstNode->uiPRI = VNETC_NID_PRI_LEARN;
    pstNode->uiIfIndex = 0;

    vnetc_node_SetSesID(pstNode, 0);

    return;
}

VOID VNETC_NODE_SesClosed(IN UINT uiNID, IN UINT uiSesID)
{
    MUTEX_P(&g_stVnetcNodeMutex);
    vnetc_node_SesClosed(uiNID, uiSesID);
    MUTEX_V(&g_stVnetcNodeMutex);
}

static VOID vnetc_node_DirectDetectFailed(IN UINT uiNID)
{
    VNETC_NID_S *pstNode;

    pstNode = VNETC_NODE_GetNode(uiNID);
    if (NULL == pstNode)
    {
        return;
    }

    if (pstNode->uiPRI < VNETC_NID_PRI_DIRETECT)
    {
        return;
    }

    if (pstNode->uiSesID != 0)  
    {
        return;
    }

    pstNode->uiPRI = VNETC_NID_PRI_LEARN;
    pstNode->uiIfIndex = 0;
    pstNode->ucDirectStatus = VNETC_NODE_DIRECT_FAILED;

    return;
}

VOID VNETC_NODE_DirectDetectFailed(IN UINT uiNID)
{
    MUTEX_P(&g_stVnetcNodeMutex);
    vnetc_node_DirectDetectFailed(uiNID);
    MUTEX_V(&g_stVnetcNodeMutex);
}

VOID VNETC_NODE_Del(IN UINT uiNID)
{
    MUTEX_P(&g_stVnetcNodeMutex);
    vnetc_node_Del(uiNID);
    MUTEX_V(&g_stVnetcNodeMutex);
}

VOID VNETC_NODE_SetSesInfo(IN UINT uiNID, IN UINT uiIfIndex, IN UINT uiSesID)
{
    VNETC_NID_S *pstNid;

    pstNid = vnetc_node_Find(uiNID);
    if (NULL == pstNid)
    {
        return;
    }

    pstNid->uiIfIndex = uiIfIndex;
    pstNid->uiSesID = uiSesID;
}

VNETC_NID_S * VNETC_NODE_GetNode(IN UINT uiNID)
{
    return vnetc_node_Find(uiNID);
}

static int vnetc_node_ShowEach(IN HASH_S * hHashId, IN VOID *pNode, IN VOID * pUserHandle)
{
    VNETC_NID_S *pstNode = pNode;
    CHAR szIfName[IF_MAX_NAME_LEN + 1];

    EXEC_OutInfo(" %-1s%-5d %08x  %-12s %s \r\n",
            VNET_NODE_GetTypeStringByNID(pstNode->uiNID),
            VNET_NID_INDEX(pstNode->uiNID),
            pstNode->uiSesID,
            vnetc_node_GetDirectString(pstNode->ucDirectStatus),
            IFNET_GetIfName(pstNode->uiIfIndex, szIfName));

    return 0;
}


PLUG_API BS_STATUS VNETC_NODE_Show(IN UINT uiArgc, IN CHAR ** argv)
{
    EXEC_OutString(" NodeID SessionID DirectStatus IfName \r\n"
                              " --------------------------------\r\n");

    MUTEX_P(&g_stVnetcNodeMutex);

    HASH_Walk(g_hVnetcNidHandle, vnetc_node_ShowEach, NULL);

    MUTEX_V(&g_stVnetcNodeMutex);

    EXEC_OutString("\r\n");

    return BS_OK;
}

