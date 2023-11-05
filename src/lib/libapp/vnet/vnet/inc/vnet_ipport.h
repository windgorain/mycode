/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-10-5
* Description: 
* History:     
******************************************************************************/

#ifndef __VNET_IPPORT_H_
#define __VNET_IPPORT_H_

#ifdef __cplusplus
    extern "C" {
#endif 


#define VNET_IPPORT_NODE_FLAG_STATIC    0x1             
#define VNET_IPPORT_NODE_FLAG_OLD       0x80000000      


typedef struct
{
    UINT  ulIp;        
    USHORT usPort;      
    UINT  ulSesId;
    UINT  ulFlag;
}VNET_IPPORT_NODE_S;

typedef VOID  (*PF_VNET_IPPORT_WALK_FUNC)(IN VNET_IPPORT_NODE_S *pstNode, IN VOID * pUserHandle);


BS_STATUS VNET_IPPORT_Init();
BS_STATUS VNET_IPPORT_Add(IN VNET_IPPORT_NODE_S *pstNode);
BS_STATUS VNET_IPPORT_Del(IN VNET_IPPORT_NODE_S *pstNode);
BS_STATUS VNET_IPPORT_Find(INOUT VNET_IPPORT_NODE_S *pstNode, IN BOOL_T bIfSetOldFlagToMax);
VOID VNET_IPPORT_Walk(IN PF_VNET_IPPORT_WALK_FUNC pfFunc, IN VOID *pUserHandle);

VOID VNET_IPPORT_Old();

#ifdef __cplusplus
    }
#endif 

#endif 


