/******************************************************************************
* Copyright (C),    LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-12-3
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mutex_utl.h"
#include "utl/nap_utl.h"
#include "utl/txt_utl.h"
#include "utl/exec_utl.h"
#include "utl/lstr_utl.h"
#include "utl/rand_utl.h"

#include "../../inc/vnet_conf.h"
#include "../../inc/vnet_node.h"

#include "../inc/vnets_phy.h"
#include "../inc/vnets_node.h"
#include "../inc/vnets_tp.h"
#include "../inc/vnets_domain.h"


#define VNETS_NODE_MAX_NUM (1024 * 10)

static HANDLE g_hVnetsNodeHandle = NULL;
static UINT g_uiVnetsNodeSelf = VNET_NID_SERVER;  
static MUTEX_S g_stVnetsNodeMutex;

static UINT vnets_node_AddNode(IN UINT uiSesID)
{
    VNETS_NODE_S *pstNode;

    pstNode = NAP_ZAlloc(g_hVnetsNodeHandle);
    if (NULL == pstNode) {
        return VNETS_NODE_INVALID_ID;
    }

    pstNode->uiSesID = uiSesID;

    pstNode->uiNodeID = (UINT)NAP_GetIDByNode(g_hVnetsNodeHandle, pstNode);
    pstNode->uiCookie = (UINT)RAND_GetNonZero();

    return pstNode->uiNodeID;
}

static VOID vnets_node_DelNode(IN UINT uiNodeID)
{
    VNETS_NODE_S *pstNode;
    
    pstNode = NAP_GetNodeByID(g_hVnetsNodeHandle, uiNodeID);
    if (NULL == pstNode)
    {
        return;
    }

    VNETS_TP_SetProperty(pstNode->uiTpID, VNETS_TP_PROPERTY_NODE, NULL);

    if (pstNode->uiDomainID != 0)
    {
        VNETS_Domain_DelNode(pstNode->uiDomainID, uiNodeID);
    }

    NAP_Free(g_hVnetsNodeHandle, pstNode);

    return;
}

static inline VOID vnets_node_CopyNodeInfo(IN VNETS_NODE_S *pstNode, OUT VNETS_NODE_INFO_S *pstNodeInfo)
{
    pstNodeInfo->uiNodeID = pstNode->uiNodeID;
    pstNodeInfo->uiIP = pstNode->uiIP;
    pstNodeInfo->uiMask = pstNode->uiMask;
    pstNodeInfo->uiSesID = pstNode->uiSesID;
    pstNodeInfo->stMACAddr = pstNode->stMACAddr;
    TXT_Strlcpy(pstNodeInfo->szUserName, pstNode->szUserName, sizeof(pstNodeInfo->szUserName));
    TXT_Strlcpy(pstNodeInfo->szAlias, pstNode->szAlias, sizeof(pstNodeInfo->szAlias));
    TXT_Strlcpy(pstNodeInfo->szDescription, pstNode->szDescription, sizeof(pstNodeInfo->szDescription));
}

static BS_STATUS vnets_node_GetNodeInfo(IN UINT uiNodeID, OUT VNETS_NODE_INFO_S *pstNodeInfo)
{
    VNETS_NODE_S *pstNode;

    pstNode = NAP_GetNodeByID(g_hVnetsNodeHandle, uiNodeID);
    if (NULL == pstNode)
    {
        return BS_NO_SUCH;
    }

    vnets_node_CopyNodeInfo(pstNode, pstNodeInfo);

    return BS_OK;
}

static VOID vnets_node_TpCloseNotify(IN UINT uiTPID, IN HANDLE *phPropertys, IN USER_HANDLE_S *pstUserHandle)
{
    UINT uiNodeID;

    uiNodeID = HANDLE_UINT(phPropertys[VNETS_TP_PROPERTY_NODE]);
    if (uiNodeID != 0)
    {
        VNETS_NODE_DelNode(uiNodeID);
    }
}

BS_STATUS VNETS_NODE_Init()
{
    NAP_PARAM_S nap_param = {0};

    nap_param.enType = NAP_TYPE_HASH;
    nap_param.uiMaxNum = VNETS_NODE_MAX_NUM;
    nap_param.uiNodeSize = sizeof(VNETS_NODE_S);

    g_hVnetsNodeHandle = NAP_Create(&nap_param);
    if (NULL == g_hVnetsNodeHandle)
    {
        return BS_ERR;
    }

    MUTEX_Init(&g_stVnetsNodeMutex);

    return BS_OK;
}

BS_STATUS VNETS_NODE_Init2()
{
    return VNETS_TP_RegCloseEvent(vnets_node_TpCloseNotify, NULL);
}

UINT VNETS_NODE_AddNode(IN UINT uiSesID)
{
    UINT uiNodeID;

    MUTEX_P(&g_stVnetsNodeMutex);
    uiNodeID = vnets_node_AddNode(uiSesID);
    MUTEX_V(&g_stVnetsNodeMutex);

    return uiNodeID;
}

VOID VNETS_NODE_DelNode(IN UINT uiNodeID)
{
    MUTEX_P(&g_stVnetsNodeMutex);
    vnets_node_DelNode(uiNodeID);
    MUTEX_V(&g_stVnetsNodeMutex);
}

BS_STATUS VNETS_NODE_SetUserName(IN UINT uiNodeID, IN CHAR *pcUserName)
{
    VNETS_NODE_S *pstNode;
    
    pstNode = NAP_GetNodeByID(g_hVnetsNodeHandle, uiNodeID);
    if (NULL == pstNode)
    {
        return BS_NO_SUCH;
    }

    TXT_Strlcpy(pstNode->szUserName, pcUserName, sizeof(pstNode->szUserName));

    return BS_OK;
}

BS_STATUS VNETS_NODE_SetTPID(IN UINT uiNodeID, IN UINT uiTPID)
{
    VNETS_NODE_S *pstNode;
    
    pstNode = NAP_GetNodeByID(g_hVnetsNodeHandle, uiNodeID);
    if (NULL == pstNode)
    {
        return BS_NO_SUCH;
    }

    if (pstNode->uiTpID != 0)
    {
        VNETS_TP_SetProperty(pstNode->uiTpID, VNETS_TP_PROPERTY_NODE, UINT_HANDLE(0));
    }

    pstNode->uiTpID = uiTPID;

    if (uiTPID != 0)
    {
        VNETS_TP_SetProperty(uiTPID, VNETS_TP_PROPERTY_NODE, UINT_HANDLE(uiNodeID));
    }

    return BS_OK;
}

BS_STATUS VNETS_NODE_SetDomainID(IN UINT uiNodeID, IN UINT uiDomainID)
{
    VNETS_NODE_S *pstNode;
    
    pstNode = NAP_GetNodeByID(g_hVnetsNodeHandle, uiNodeID);
    if (NULL == pstNode)
    {
        return BS_NO_SUCH;
    }

    pstNode->uiDomainID = uiDomainID;

    return BS_OK;
}

BS_STATUS VNETS_NODE_SetNodeInfo(IN UINT uiNodeID, IN VNETS_NODE_INFO_S *pstInfo)
{
    VNETS_NODE_S *pstNode;
    
    pstNode = NAP_GetNodeByID(g_hVnetsNodeHandle, uiNodeID);
    if (NULL == pstNode)
    {
        return BS_NO_SUCH;
    }

    MUTEX_P(&g_stVnetsNodeMutex);
    pstNode->uiIP = pstInfo->uiIP;
    pstNode->uiMask = pstInfo->uiMask;
    pstNode->stMACAddr = pstInfo->stMACAddr;
    TXT_Strlcpy(pstNode->szDescription, pstInfo->szDescription, sizeof(pstNode->szDescription));
    TXT_Strlcpy(pstNode->szAlias, pstInfo->szAlias, sizeof(pstNode->szAlias));
    MUTEX_V(&g_stVnetsNodeMutex);

    return BS_OK;
}

VNETS_NODE_S * VNETS_NODE_GetNode(IN UINT uiNodeID)
{
    return NAP_GetNodeByID(g_hVnetsNodeHandle, uiNodeID);
}

UINT VNETS_NODE_GetCookie(IN UINT uiNodeID)
{
    VNETS_NODE_S * pstNode;

    pstNode = VNETS_NODE_GetNode(uiNodeID);
    if (NULL == pstNode)
    {
        return 0;
    }

    return pstNode->uiCookie;
}

BS_STATUS VNETS_NODE_GetCookieString(IN UINT uiNodeID, OUT CHAR szCookieString[VNETS_NODE_COOKIE_STRING_LEN + 1])
{
    UINT uiCookie;

    uiCookie = VNETS_NODE_GetCookie(uiNodeID);
    if (uiCookie == 0)
    {
        return BS_ERR;
    }

    snprintf(szCookieString, VNETS_NODE_COOKIE_STRING_LEN + 1, "%x-%x", uiCookie, uiNodeID);

    return BS_OK;
}


UINT VNETS_NODE_GetNodeIdByCookieString(IN CHAR *pcCookieString)
{
    LSTR_S stStrCookie;
    LSTR_S stStrNodeID;
    UINT uiNodeID;
    UINT uiCookie;
    UINT uiCookieSaved;

    TXT_StrSplit(pcCookieString, '-', &stStrCookie, &stStrNodeID);
    if ((stStrCookie.uiLen == 0) || (stStrNodeID.uiLen == 0))
    {
        return 0;
    }

    if (BS_OK != LSTR_XAtoui(&stStrNodeID, &uiNodeID))
    {
        return 0;
    }

    if (BS_OK != LSTR_XAtoui(&stStrCookie, &uiCookie))
    {
        return 0;
    }

    uiCookieSaved = VNETS_NODE_GetCookie(uiNodeID);
    if (0 == uiCookieSaved)
    {
        return 0;
    }

    if (uiCookieSaved != uiCookie)
    {
        return 0;
    }

    return uiNodeID;
}

UINT VNETS_NODE_GetDomainID(IN UINT uiNodeID)
{
    VNETS_NODE_S * pstNode;

    pstNode = VNETS_NODE_GetNode(uiNodeID);
    if (NULL == pstNode)
    {
        return 0;
    }

    return pstNode->uiDomainID;
}

UINT VNETS_NODE_GetSesID(IN UINT uiNodeID)
{
    VNETS_NODE_S * pstNode;

    pstNode = VNETS_NODE_GetNode(uiNodeID);
    if (NULL == pstNode)
    {
        return 0;
    }

    return pstNode->uiSesID;
}

BS_STATUS VNETS_NODE_GetNodeInfo(IN UINT uiNodeID, OUT VNETS_NODE_INFO_S *pstNodeInfo)
{
    BS_STATUS eRet;
    
    MUTEX_P(&g_stVnetsNodeMutex);
    eRet = vnets_node_GetNodeInfo(uiNodeID, pstNodeInfo);
    MUTEX_V(&g_stVnetsNodeMutex);

    return eRet;
}

UINT VNETS_NODE_GetNext(IN UINT uiCurrentID)
{
    UINT uiNextID;

    MUTEX_P(&g_stVnetsNodeMutex);
    uiNextID = (UINT)NAP_GetNextID(g_hVnetsNodeHandle, uiCurrentID);
    MUTEX_V(&g_stVnetsNodeMutex);

    return uiNextID;
}

VOID VNETS_NODE_Walk(IN PF_VNETS_NODE_WALK_EACH pfFunc, IN HANDLE hUserHandle)
{
    UINT index = NAP_INVALID_INDEX;
    UINT id;

    MUTEX_P(&g_stVnetsNodeMutex);

    while (NAP_INVALID_INDEX !=
            (index = NAP_GetNextIndex(g_hVnetsNodeHandle, index))) {
        id = NAP_GetIDByIndex(g_hVnetsNodeHandle, index);
        if (pfFunc(id, hUserHandle) < 0) {
            break;
        }
    }

    MUTEX_V(&g_stVnetsNodeMutex);
}

UINT VNETS_NODE_Self()
{
    return g_uiVnetsNodeSelf;
}

PLUG_API BS_STATUS VNETS_NODE_Show(IN UINT uiArgc, IN CHAR ** argv)
{
    VNETS_NODE_S *pstNode;
    UINT uiIndex = NAP_INVALID_ID;

    EXEC_OutString(" NodeID   Domain SesID    TPID     IP              Description \r\n"
                              " --------------------------------------------------------------------\r\n");

    MUTEX_P(&g_stVnetsNodeMutex);

    while ((uiIndex = NAP_GetNextIndex(g_hVnetsNodeHandle, uiIndex))
            != NAP_INVALID_INDEX) {
        pstNode = NAP_GetNodeByIndex(g_hVnetsNodeHandle, uiIndex);

        EXEC_OutInfo(" %-8x %-6d %-8x %-8x %-15pI4 %s \r\n",
            pstNode->uiNodeID, pstNode->uiDomainID, pstNode->uiSesID,
            pstNode->uiTpID, &pstNode->uiIP, pstNode->szDescription);
    }

    MUTEX_V(&g_stVnetsNodeMutex);

    EXEC_OutString("\r\n");

    return BS_OK;
}

