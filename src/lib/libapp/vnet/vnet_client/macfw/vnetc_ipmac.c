/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-10-13
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/ipmac_tbl.h"
#include "utl/mutex_utl.h"

#define VNET_CLIENT_IPMAC_MAX_AGE 3

static IPMAC_HANDLE g_hVnetClientIpMacTbl = NULL;
static MUTEX_S g_stVnetClientIpMacTblMutex;

BS_STATUS VNETC_Ipmac_Init()
{
    g_hVnetClientIpMacTbl = IPMAC_TBL_CreateInstance();
    if (NULL == g_hVnetClientIpMacTbl)
    {
        return BS_NO_MEMORY;
    }

    MUTEX_Init(&g_stVnetClientIpMacTblMutex);

    return BS_OK;
}

VOID VNETC_Ipmac_Fin()
{
    IPMAC_TBL_DelInstance(g_hVnetClientIpMacTbl);
}

BS_STATUS VNETC_Ipmac_Add(IN UINT uiIp, IN MAC_ADDR_S *pstMac)
{
    BS_STATUS enStatus = BS_ERR;
    IPMAC_TBL_NODE_S *pstIpMacNode;
    
    MUTEX_P(&g_stVnetClientIpMacTblMutex);
    pstIpMacNode = IPMAC_TBL_Add(g_hVnetClientIpMacTbl, uiIp, pstMac);
    if (NULL != pstIpMacNode)
    {
        pstIpMacNode->uiUsrContext = VNET_CLIENT_IPMAC_MAX_AGE;
        enStatus = BS_OK;
    }
    MUTEX_V(&g_stVnetClientIpMacTblMutex);

    return enStatus;
}

VOID VNETC_Ipmac_Del(IN UINT uiIp)
{
    MUTEX_P(&g_stVnetClientIpMacTblMutex);
    IPMAC_TBL_Del(g_hVnetClientIpMacTbl, uiIp);
    MUTEX_V(&g_stVnetClientIpMacTblMutex);
}

MAC_ADDR_S * VNETC_Ipmac_GetMacByIp(IN UINT uiIp, OUT MAC_ADDR_S *pstMac)
{
    IPMAC_TBL_NODE_S *pstIpMacNode;

    MUTEX_P(&g_stVnetClientIpMacTblMutex);
    pstIpMacNode = IPMAC_TBL_Find(g_hVnetClientIpMacTbl, uiIp);
    if (NULL != pstIpMacNode)
    {
        *pstMac = pstIpMacNode->stMac;
        pstIpMacNode->uiUsrContext --;
        if (0 == pstIpMacNode->uiUsrContext)
        {
            IPMAC_TBL_Del(g_hVnetClientIpMacTbl, uiIp);
        }
    }
    MUTEX_V(&g_stVnetClientIpMacTblMutex);

    if (NULL == pstIpMacNode)
    {
        return NULL;
    }

    return pstMac;
}

static VOID vnetc_ipmac_ShowEach(IN IPMAC_TBL_NODE_S *pstNode, IN VOID * pUserHandle)
{
    EXEC_OutInfo(" %-15s  %02x-%02x-%02x-%02x-%02x-%02x  %d\r\n",
        Socket_IpToName(ntohl(pstNode->uiIp)),
        pstNode->stMac.aucMac[0],
        pstNode->stMac.aucMac[1],
        pstNode->stMac.aucMac[2],
        pstNode->stMac.aucMac[3],
        pstNode->stMac.aucMac[4],
        pstNode->stMac.aucMac[5],
        pstNode->uiUsrContext);
}

PLUG_API BS_STATUS VNETC_Ipmac_Show()
{
    EXEC_OutString(" IP               MAC                Age\r\n"
        "--------------------------------------------\r\n");

    MUTEX_P(&g_stVnetClientIpMacTblMutex);
    IPMAC_TBL_Walk(g_hVnetClientIpMacTbl, vnetc_ipmac_ShowEach, NULL);
    MUTEX_V(&g_stVnetClientIpMacTblMutex);

    EXEC_Flush(0);

	return BS_OK;
}


