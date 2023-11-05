/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-9-20
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mac_table.h"
#include "utl/mutex_utl.h"
#include "utl/txt_utl.h"
#include "utl/exec_utl.h"

#include "../../inc/vnet_conf.h"

#include "../inc/vnets_conf.h"
#include "../inc/vnets_phy.h"
#include "../inc/vnets_ses.h"
#include "../inc/vnets_domain.h"
#include "../inc/vnets_mac_tbl.h"
#include "../inc/vnets_dc.h"
#include "../inc/vnets_rcu.h"
#include "../inc/vnets_node.h"

typedef struct
{
    MUTEX_S stMutex;
    MACTBL_HANDLE hMacTbl;
}VNET_SERVER_MACTBL_S;

static BS_STATUS vnets_mactbl_DomainEventCreate(IN VNETS_DOMAIN_RECORD_S *pstParam)
{
    VNET_SERVER_MACTBL_S *pstMacTbl;
    MACTBL_HANDLE hMacTbl;

    hMacTbl = MACTBL_CreateInstance(sizeof(VNETS_MAC_USER_DATA_S));
    if (NULL == hMacTbl) {
        return BS_NO_MEMORY;
    }
    MACTBL_SetOldTick(hMacTbl, 2);

    pstMacTbl = MEM_ZMalloc(sizeof(VNET_SERVER_MACTBL_S));
    if (NULL == pstMacTbl)
    {
        MACTBL_DestoryInstance(hMacTbl);
        return BS_NO_MEMORY;
    }

    MUTEX_Init(&pstMacTbl->stMutex);

    pstMacTbl->hMacTbl = hMacTbl;

    pstParam->ahProperty[VNETS_DOMAIN_PROPERTY_MACTBL] = pstMacTbl;

    return BS_OK;
}

static VOID vnets_mactbl_DomainEventDelete(IN VNETS_DOMAIN_RECORD_S *pstParam)
{
    VNET_SERVER_MACTBL_S *pstMacTbl;

    pstMacTbl = pstParam->ahProperty[VNETS_DOMAIN_PROPERTY_MACTBL];

    if (NULL == pstMacTbl)
    {
        return;
    }

    pstParam->ahProperty[VNETS_DOMAIN_PROPERTY_MACTBL] = NULL;

    if (pstMacTbl->hMacTbl != NULL)
    {
        MACTBL_DestoryInstance(pstMacTbl->hMacTbl);
    }

    MUTEX_Final(&pstMacTbl->stMutex);

    MEM_Free(pstMacTbl);
}

static VNET_SERVER_MACTBL_S * vnets_mactbl_GetMacTbl(IN UINT uiDomainID)
{
    VNET_SERVER_MACTBL_S *pstMacTbl = NULL;

    if (uiDomainID == 0)
    {
        return NULL;
    }

    if (BS_OK != VNETS_Domain_GetProperty(uiDomainID, VNETS_DOMAIN_PROPERTY_MACTBL, (HANDLE*)&pstMacTbl))
    {
        return NULL;
    }

    return pstMacTbl;
}

BS_STATUS VNETS_MACTBL_DomainEventInput
(
    IN VNETS_DOMAIN_RECORD_S *pstParam,
    IN UINT uiEvent
)
{
    BS_STATUS eRet = BS_OK;

    switch (uiEvent)
    {
        case VNET_DOMAIN_EVENT_AFTER_CREATE:
        {
            eRet = vnets_mactbl_DomainEventCreate(pstParam);
            break;
        }

        case VNET_DOMAIN_EVENT_PRE_DESTROY:
        {
            vnets_mactbl_DomainEventDelete(pstParam);
            break;
        }

        default:
        {
            break;
        }
    }

    return eRet;
}

BS_STATUS VNETS_MACTBL_Add(IN UINT uiDomainID, IN VNETS_MAC_NODE_S *pstMacNode, IN MAC_NODE_MODE_E eMode)
{
    BS_STATUS eRet;
    VNET_SERVER_MACTBL_S *pstMacTbl;
    UINT uiPhase;

    uiPhase = VNETS_RCU_Lock();
    pstMacTbl = vnets_mactbl_GetMacTbl(uiDomainID);
    if (NULL == pstMacTbl)
    {
        eRet = BS_ERR;
    }
    else
    {
        MUTEX_P(&pstMacTbl->stMutex);
        eRet = MACTBL_Add(pstMacTbl->hMacTbl, &pstMacNode->stMacNode, &pstMacNode->stUserData, eMode);
        MUTEX_V(&pstMacTbl->stMutex);
    }
    VNETS_RCU_UnLock(uiPhase);

    return eRet;
}

BS_STATUS VNETS_MACTBL_Del(IN UINT uiDomainID, IN VNETS_MAC_NODE_S *pstMacNode)
{
    BS_STATUS eRet;
    VNET_SERVER_MACTBL_S *pstMacTbl;
    UINT uiPhase;

    uiPhase = VNETS_RCU_Lock();
    pstMacTbl = vnets_mactbl_GetMacTbl(uiDomainID);
    if (NULL == pstMacTbl)
    {
        eRet = BS_ERR;
    }
    else
    {
        MUTEX_P(&pstMacTbl->stMutex);
        eRet = MACTBL_Del(pstMacTbl->hMacTbl, &pstMacNode->stMacNode);
        MUTEX_V(&pstMacTbl->stMutex);
    }
    VNETS_RCU_UnLock(uiPhase);

    return eRet;
}

BS_STATUS VNETS_MACTBL_Find(IN UINT uiDomainID, INOUT VNETS_MAC_NODE_S *pstMacNode)
{
    BS_STATUS eRet;
    VNET_SERVER_MACTBL_S *pstMacTbl;

    pstMacTbl = vnets_mactbl_GetMacTbl(uiDomainID);
    if (NULL == pstMacTbl)
    {
        return BS_ERR;
    }
    
    MUTEX_P(&pstMacTbl->stMutex);
    eRet = MACTBL_Find(pstMacTbl->hMacTbl, &pstMacNode->stMacNode, &pstMacNode->stUserData);
    MUTEX_V(&pstMacTbl->stMutex);

    return eRet;
}

static VOID vnets_mactbl_WalkEach
(
    IN HANDLE hMacTblId,
    IN MAC_NODE_S *pstNode,
    IN VOID *pUserData,
    IN VOID * pUserHandle
)
{
    PF_VNETS_MACTBL_WALK_FUNC pfFunc;
    VNETS_MAC_NODE_S stMacNode;
    USER_HANDLE_S *pstUserHandle = pUserHandle;

    pfFunc = (PF_VNETS_MACTBL_WALK_FUNC)pstUserHandle->ahUserHandle[0];

    stMacNode.stMacNode = *pstNode;
    stMacNode.stUserData = *((VNETS_MAC_USER_DATA_S*)pUserData);

    pfFunc (&stMacNode, (VOID*)(pstUserHandle->ahUserHandle[1]));
}

VOID VNETS_MACTBL_Walk(IN UINT uiDomainID, IN PF_VNETS_MACTBL_WALK_FUNC pfFunc, IN VOID *pUserHandle)
{
    USER_HANDLE_S stUserHandle;  
    VNET_SERVER_MACTBL_S *pstMacTbl;
    UINT uiPhase;

    uiPhase = VNETS_RCU_Lock();
    pstMacTbl = vnets_mactbl_GetMacTbl(uiDomainID);
    if (NULL != pstMacTbl)
    {
        stUserHandle.ahUserHandle[0] = pfFunc;
        stUserHandle.ahUserHandle[1] = pUserHandle;

        MUTEX_P(&pstMacTbl->stMutex);
        MACTBL_Walk(pstMacTbl->hMacTbl, (PF_MACTBL_WALK_FUNC)vnets_mactbl_WalkEach, &stUserHandle);
        MUTEX_V(&pstMacTbl->stMutex);
    }
    VNETS_RCU_UnLock(uiPhase);
}

static VOID vnets_mactbl_ShowEach(IN VNETS_MAC_NODE_S *pstMacNode, IN VOID * pUserHandle)
{
    EXEC_OutInfo(" %pM  %-6d   0x%08x\r\n",
        &pstMacNode->stMacNode.stMac,
        pstMacNode->stUserData.uiNodeID,
        pstMacNode->stMacNode.uiFlag);
}

static VOID vnets_mactbl_Show(IN UINT uiDomainId)
{
    EXEC_OutString(" MAC                NodeID   Flag\r\n"
        "--------------------------------------------\r\n");

    VNETS_MACTBL_Walk(uiDomainId, vnets_mactbl_ShowEach, NULL);

    EXEC_OutString("\r\n");

    return;
}


PLUG_API BS_STATUS VNETS_MACTBL_ShowByDomainName(IN UINT ulArgc, IN CHAR ** argv)
{
    UINT uiDomainId;

    if (ulArgc < 4)
    {
        return BS_BAD_PARA;
    }

    uiDomainId = VNETS_Domain_GetDomainID(argv[3]);
    if (uiDomainId != 0)
    {
        vnets_mactbl_Show(uiDomainId);
    }

    return BS_OK;
}


PLUG_API BS_STATUS VNETS_MACTBL_ShowByDomainID(IN UINT ulArgc, IN CHAR ** argv)
{
    UINT uiDomainId = 0;

    if (ulArgc < 4)
    {
        return BS_BAD_PARA;
    }

    TXT_Atoui(argv[3], &uiDomainId);

    if (uiDomainId != 0)
    {
        vnets_mactbl_Show(uiDomainId);
    }

    return BS_OK;
}


