/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-9-23
* Description: 
* History:     
******************************************************************************/

#ifndef __VNETS_MAC_TBL_H_
#define __VNETS_MAC_TBL_H_

#include "utl/mac_table.h"

#include "../inc/vnets_domain.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef struct
{
    UINT uiNodeID;
}VNETS_MAC_USER_DATA_S;

typedef struct
{
    MAC_NODE_S stMacNode;
    VNETS_MAC_USER_DATA_S stUserData;
}VNETS_MAC_NODE_S;

typedef VOID  (*PF_VNETS_MACTBL_WALK_FUNC)(IN VNETS_MAC_NODE_S *pstNode, IN VOID * pUserHandle);

BS_STATUS VNETS_MACTBL_DomainEventInput
(
    IN VNETS_DOMAIN_RECORD_S *pstParam,
    IN UINT uiEvent
);
BS_STATUS VNETS_MACTBL_Add(IN UINT uiDomainID, IN VNETS_MAC_NODE_S *pstMacNode, IN MAC_NODE_MODE_E eMode);
BS_STATUS VNETS_MACTBL_Del(IN UINT uiDomainID, IN VNETS_MAC_NODE_S *pstMacNode);
BS_STATUS VNETS_MACTBL_Find(IN UINT uiDomainID, INOUT VNETS_MAC_NODE_S *pstMacNode);
VOID VNETS_MACTBL_Walk(IN UINT uiDomainID, IN PF_VNETS_MACTBL_WALK_FUNC pfFunc, IN VOID *pUserHandle);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETS_MAC_TBL_H_*/


