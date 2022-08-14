/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-10-9
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"

#include "../inc/vnets_master.h"
#include "../inc/vnets_domain.h"
#include "../inc/vnets_node.h"
#include "../inc/vnets_rmt.h"

static VOID vnets_rmt_DomainReboot(void *ud)
{
    CHAR *pcDomainName = ud;

    VNETS_Domain_RebootByName(pcDomainName);

    MEM_Free(pcDomainName);
}

int VNETS_RMT_DomainReboot(U64 p1)
{
    CHAR *pcDomainName = (void*)p1;
    CHAR *pcMem;

    pcMem = TXT_Strdup(pcDomainName);
    if (NULL == pcMem)
    {
        return BS_NO_MEMORY;
    }

    if (BS_OK != VNETS_Master_MsgInput(vnets_rmt_DomainReboot, pcMem))
    {
        MEM_Free(pcMem);
        return BS_ERR;
    }

    return BS_OK;
}

UINT VNETS_RMT_DomainGetNextNode(U64 p1, U64 p2)
{
    CHAR *pcDomainName = (void*)p1;
    UINT uiCurrentNodeId = p2;

    return VNETS_Domain_GetNextNode(pcDomainName, uiCurrentNodeId);
}

HSTRING VNETS_RMT_GetNodeInfo(U64 p1)
{
    UINT uiNodeID = p1;
    HSTRING hInfo;
    CHAR szInfo[32];
    VNETS_NODE_INFO_S stNodeInfo;

    if (BS_OK != VNETS_NODE_GetNodeInfo(uiNodeID, &stNodeInfo))
    {
        return NULL;
    }

    hInfo = STRING_Create();
    if (NULL == hInfo)
    {
        return NULL;
    }

    STRING_CatFromBuf(hInfo, stNodeInfo.szDescription);
    STRING_CatFromBuf(hInfo, ";");

    BS_Snprintf(szInfo, sizeof(szInfo), "%pI4", &stNodeInfo.uiIP);
    STRING_CatFromBuf(hInfo, szInfo);
    STRING_CatFromBuf(hInfo, ";");

    BS_Snprintf(szInfo, sizeof(szInfo), "%pI4", &stNodeInfo.uiMask);
    STRING_CatFromBuf(hInfo, szInfo);
    STRING_CatFromBuf(hInfo, ";");

    BS_Snprintf(szInfo, sizeof(szInfo), "%pM", &stNodeInfo.stMACAddr);
    STRING_CatFromBuf(hInfo, szInfo);

    return hInfo;
}

