/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-9-23
* Description: 
* History:     
******************************************************************************/

#ifndef __VNETC_MAC_TBL_H_
#define __VNETC_MAC_TBL_H_

#include "utl/mac_table.h"

#ifdef __cplusplus
    extern "C" {
#endif 

typedef struct
{
    UINT uiNodeID;
}VNETC_MAC_USER_DATA_S;

typedef enum
{
    VNETC_MAC_PRI_LOW = 0,
    VNETC_MAC_PRI_NORMAL = 1000,
    VNETC_MAC_PRI_HIGH = 2000
}VNETC_MAC_PRI_E;

typedef VOID  (*PF_VNETC_MACTBL_WALK_FUNC)
(
    IN MAC_NODE_S *pstMacNode,
    IN VNETC_MAC_USER_DATA_S *pstUserData,
    IN VOID * pUserHandle
);

BS_STATUS VNETC_MACTBL_Init();
BS_STATUS VNETC_MACTBL_Add(IN MAC_NODE_S *pstMacNode, IN VNETC_MAC_USER_DATA_S *pstUserData, IN MAC_NODE_MODE_E eMode);
BS_STATUS VNETC_MACTBL_Del(IN MAC_NODE_S *pstMacNode);
BS_STATUS VNETC_MACTBL_Find(INOUT MAC_NODE_S *pstMacNode, OUT VNETC_MAC_USER_DATA_S *pstUserData);
VOID VNETC_MACTBL_Walk(IN PF_VNETC_MACTBL_WALK_FUNC pfFunc, IN VOID *pUserHandle);

#ifdef __cplusplus
    }
#endif 

#endif 


