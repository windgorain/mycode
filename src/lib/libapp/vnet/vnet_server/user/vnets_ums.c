/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-5-23
* Description: vnets ueer mac ses map;
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/hash_utl.h"
#include "utl/txt_utl.h"
#include "utl/eth_utl.h"

#include "../../inc/vnet_conf.h"

#include "vnets_ums.h"

#define _VNETS_UMS_HASH_BUCKET 1024

typedef struct
{
    HASH_NODE_S stHashNode;
    CHAR szUserName[VNET_CONF_MAX_USER_NAME_LEN + 1];
    MAC_ADDR_S stMacAddr;
    UINT uiSesId;
}_VNETS_UMS_NODE_S;

static HASH_HANDLE g_hVnetsUmsHash = NULL;

static UINT vnets_ums_HashIndex(IN VOID *pNode)
{
    _VNETS_UMS_NODE_S *pstUmsNode = (_VNETS_UMS_NODE_S *)pNode;
    UINT uiIndex = 0;
    CHAR *pcChar;
    UINT i;

    pcChar = pstUmsNode->szUserName;
    while (*pcChar != '\0')
    {
        uiIndex += *pcChar;
        pcChar ++;
    }

    for (i=0; i<MAC_ADDR_LEN; i++)
    {
        uiIndex = pstUmsNode->stMacAddr.aucMac[i];
    }

    return uiIndex;
}

static INT vnets_ums_Cmp(IN VOID *pNode1, IN VOID *pNode2)
{
    _VNETS_UMS_NODE_S *pstNode1 = (_VNETS_UMS_NODE_S *)pNode1;
    _VNETS_UMS_NODE_S *pstNode2 = (_VNETS_UMS_NODE_S *)pNode2;
    
    INT iCmp;

    iCmp = stricmp(pstNode1->szUserName, pstNode2->szUserName);
    if (iCmp != 0)
    {
        return iCmp;
    }

    iCmp = MEM_Cmp(pstNode1->stMacAddr.aucMac, MAC_ADDR_LEN, pstNode2->stMacAddr.aucMac, MAC_ADDR_LEN);

    return iCmp;
}

static _VNETS_UMS_NODE_S * vnets_ums_Find
(
    IN CHAR *pcUserName,
    IN MAC_ADDR_S *pstMac
)
{
    _VNETS_UMS_NODE_S stNode;

    TXT_Strlcpy(stNode.szUserName, pcUserName, sizeof(stNode.szUserName));
    stNode.stMacAddr = *pstMac;

    return HASH_Find(g_hVnetsUmsHash, vnets_ums_Cmp, &stNode);
}

BS_STATUS _VNETS_UMS_Init()
{
    g_hVnetsUmsHash = HASH_CreateInstance(NULL, _VNETS_UMS_HASH_BUCKET, vnets_ums_HashIndex);
    if (NULL == g_hVnetsUmsHash)
    {
        return BS_NO_MEMORY;
    }

    return BS_OK;
}

BS_STATUS _VNETS_UMS_Add
(
    IN CHAR *pcUserName,
    IN MAC_ADDR_S *pstMac,
    IN UINT uiSesId
)
{
    _VNETS_UMS_NODE_S * pstNode;

    if (NULL != vnets_ums_Find(pcUserName, pstMac))
    {
        return BS_ALREADY_EXIST;
    }

    pstNode = MEM_ZMalloc(sizeof(_VNETS_UMS_NODE_S));
    if (NULL == pstNode)
    {
        return BS_NO_MEMORY;
    }

    TXT_Strlcpy(pstNode->szUserName, pcUserName, sizeof(pstNode->szUserName));
    pstNode->stMacAddr = *pstMac;
    pstNode->uiSesId = uiSesId;

    HASH_Add(g_hVnetsUmsHash, pstNode);

    return BS_OK;
}

VOID _VNETS_UMS_Del
(
    IN CHAR *pcUserName,
    IN MAC_ADDR_S *pstMac
)
{
    _VNETS_UMS_NODE_S *pstNode;

    pstNode = vnets_ums_Find(pcUserName, pstMac);
    if (NULL == pstNode)
    {
        return;
    }

    HASH_Del(g_hVnetsUmsHash, pstNode);
    MEM_Free(pstNode);
}

UINT _VNETS_UMS_GetSesId
(
    IN CHAR *pcUserName,
    IN MAC_ADDR_S *pstMac
)
{
    _VNETS_UMS_NODE_S *pstNode;
    UINT uiSesId = 0;

    pstNode = vnets_ums_Find(pcUserName, pstMac);
    if (NULL != pstNode)
    {
        uiSesId = pstNode->uiSesId;
    }

    return uiSesId;
}

