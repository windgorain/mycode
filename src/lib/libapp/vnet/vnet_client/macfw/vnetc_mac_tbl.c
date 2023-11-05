/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-9-20
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mac_table.h"
#include "utl/mutex_utl.h"
#include "utl/mbuf_utl.h"

#include "../../inc/vnet_node.h"

#include "../inc/vnetc_mac_tbl.h"
#include "../inc/vnetc_protocol.h"
#include "../inc/vnetc_master.h"

#define _VNETC_MAC_TBL_TIMEOUT_TICK  5      
#define _VNETC_MAC_TBL_TIME_PER_TICK 60000  


static MACTBL_HANDLE g_hVnetcMacTblId = 0;
static MUTEX_S g_stVnetcMactTblMutex;

static VOID vnetc_mactbl_TimeOut(IN HANDLE hTimerId, IN USER_HANDLE_S *pstUserHandle)
{
    MUTEX_P(&g_stVnetcMactTblMutex);
    MACTBL_TickStep(g_hVnetcMacTblId);
    MUTEX_V(&g_stVnetcMactTblMutex);
}

BS_STATUS VNETC_MACTBL_Init()
{
    MUTEX_Init(&g_stVnetcMactTblMutex);
    
    g_hVnetcMacTblId = MACTBL_CreateInstance(sizeof(VNETC_MAC_USER_DATA_S), _VNETC_MAC_TBL_TIMEOUT_TICK);
    if (0 == g_hVnetcMacTblId)
    {
        MUTEX_Final(&g_stVnetcMactTblMutex);
        return(BS_ERR);
    }

    VNETC_Master_AddTimer(_VNETC_MAC_TBL_TIME_PER_TICK, TIMER_FLAG_CYCLE, vnetc_mactbl_TimeOut, NULL);
 
    return BS_OK;
}

BS_STATUS VNETC_MACTBL_Add(IN MAC_NODE_S *pstMacNode, IN VNETC_MAC_USER_DATA_S *pstUserData, IN MAC_NODE_MODE_E eMode)
{
    BS_STATUS eRet;
    
    MUTEX_P(&g_stVnetcMactTblMutex);
    eRet = MACTBL_Add(g_hVnetcMacTblId, pstMacNode, pstUserData, eMode);
    MUTEX_V(&g_stVnetcMactTblMutex);

    return eRet;
}

BS_STATUS VNETC_MACTBL_Del(IN MAC_NODE_S *pstMacNode)
{
    BS_STATUS eRet;
    
    MUTEX_P(&g_stVnetcMactTblMutex);
    eRet = MACTBL_Del(g_hVnetcMacTblId, pstMacNode);
    MUTEX_V(&g_stVnetcMactTblMutex);

    return eRet;
}

BS_STATUS VNETC_MACTBL_Find(INOUT MAC_NODE_S *pstMacNode, OUT VNETC_MAC_USER_DATA_S *pstUserData)
{
    BS_STATUS eRet;
    
    MUTEX_P(&g_stVnetcMactTblMutex);
    eRet = MACTBL_Find(g_hVnetcMacTblId, pstMacNode, pstUserData);
    MUTEX_V(&g_stVnetcMactTblMutex);

    return eRet;
}

static VOID vnetc_mactbl_WalkEach
(
    IN HANDLE hMacTblId,
    IN MAC_NODE_S *pstNode,
    IN VOID *pUserData,
    IN VOID * pUserHandle
)
{
    PF_VNETC_MACTBL_WALK_FUNC pfFunc;
    USER_HANDLE_S *pstUserHandle = pUserHandle;

    pfFunc = (PF_VNETC_MACTBL_WALK_FUNC)pstUserHandle->ahUserHandle[0];

    pfFunc (pstNode, pUserData, (VOID*)(pstUserHandle->ahUserHandle[1]));
}

VOID VNETC_MACTBL_Walk(IN PF_VNETC_MACTBL_WALK_FUNC pfFunc, IN VOID *pUserHandle)
{
    USER_HANDLE_S stUserHandle;

    stUserHandle.ahUserHandle[0] = pfFunc;
    stUserHandle.ahUserHandle[1] = pUserHandle;

    MUTEX_P(&g_stVnetcMactTblMutex);
    MACTBL_Walk(g_hVnetcMacTblId, vnetc_mactbl_WalkEach, &stUserHandle);
    MUTEX_V(&g_stVnetcMactTblMutex);
}

static VOID vnetc_mactbl_ShowEach(IN MAC_NODE_S *pstMacNode, IN VNETC_MAC_USER_DATA_S *pstUserData, IN VOID * pUserHandle)
{
    CHAR *pcType = VNET_NODE_GetTypeStringByNID(pstUserData->uiNodeID);

    if (pcType[0] == '\0')
    {
        EXEC_OutInfo(" %pM  0x%08x %-7d\r\n",
            &pstMacNode->stMac,
            pstMacNode->uiFlag,
            VNET_NID_INDEX(pstUserData->uiNodeID));
    }
    else
    {
        EXEC_OutInfo(" %pM  0x%08x %s%-6d\r\n",
            &pstMacNode->stMac,
            pstMacNode->uiFlag,
            pcType,
            VNET_NID_INDEX(pstUserData->uiNodeID));
    }
}


PLUG_API BS_STATUS VNETC_MACTBL_Show(IN UINT ulArgc, IN CHAR ** argv)
{
    EXEC_OutString(" MAC                Flag       NodeID \r\n"
        "--------------------------------------------\r\n");

    VNETC_MACTBL_Walk(vnetc_mactbl_ShowEach, NULL);

    EXEC_OutString("\r\n");

    return BS_OK;
}


