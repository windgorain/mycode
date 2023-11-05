/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2013-1-8
* Description: 
* History:     
******************************************************************************/

#ifndef __NAT_INNER_H_
#define __NAT_INNER_H_

#include "utl/bitmap1_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif 

#define _NAT_GET_TICK_BY_TIME(uiTime, uiMsInTick) (((uiTime) + (uiMsInTick) - 1) / (uiMsInTick))

#if 1
typedef struct
{
    UINT auiPubIp[NAT_MAX_PUB_IP_NUM];  

    VCLOCK_INSTANCE_HANDLE hVClock;

    USHORT usMinPort;   
    USHORT usMaxPort;   

    HASH_HANDLE hPrivateHashHandle;  
    HASH_HANDLE hPubHashHandle;      

    
    BITMAP_S stTcpPortBitMap;

    UINT uiTcpTimeOutTick;
    UINT uiSynTcpTimeOutTick;

    BOOL_T bCreateMutex;
    MUTEX_S stMutex;
}_NAT_TCP_CTRL_S;


BS_STATUS _NAT_TCP_Init
(
    IN _NAT_TCP_CTRL_S *pstTcpCtrl,
    IN USHORT usMinPort,   
    IN USHORT usMaxPort,   
    IN UINT   uiMsInTick,  
    IN BOOL_T bCreateMutex
);
VOID _NAT_TCP_Fini(IN _NAT_TCP_CTRL_S *pstTcpCtrl);
BS_STATUS _NAT_TCP_PktIn
(
    IN _NAT_TCP_CTRL_S *pstTcpCtrl,
    IN IP_HEAD_S  *pstIpHead,
    IN UCHAR *pucData,
    IN UINT uiDataLen,
    IN BOOL_T bFromPub,
    INOUT UINT *puiDomainId
);
BS_STATUS _NAT_TCP_SetPubIp
(
    IN _NAT_TCP_CTRL_S *pstCtrl,
    IN UINT auiPubIp[NAT_MAX_PUB_IP_NUM] 
);
VOID _NAT_TCP_TimerStep(IN _NAT_TCP_CTRL_S *pstCtrl);
VOID _NAT_TCP_Walk
(
    IN _NAT_TCP_CTRL_S *pstCtrl,
    IN PF_NAT_WALK_CALL_BACK_FUNC pfFunc,
    IN HANDLE hUserHandle
);

#endif



#if 1

typedef struct
{
    UINT auiPubIp[NAT_MAX_PUB_IP_NUM];  

    VCLOCK_INSTANCE_HANDLE hVClock;

    USHORT usMinPort;   
    USHORT usMaxPort;   

    HASH_HANDLE hPrivateHashHandle;  
    HASH_HANDLE hPubHashHandle;      

    
    BITMAP_S stUdpPortBitMap;

    UINT uiUdpTimeOutTick;
    UINT uiSynUdpTimeOutTick;

    BOOL_T bCreateMutex;
    MUTEX_S stMutex;
}_NAT_UDP_CTRL_S;

BS_STATUS _NAT_UDP_Init
(
    IN _NAT_UDP_CTRL_S *pstCtrl,
    IN USHORT usMinPort,   
    IN USHORT usMaxPort,   
    IN UINT   uiMsInTick,  
    IN BOOL_T bCreateMutex
);
VOID _NAT_UDP_Fini(IN _NAT_UDP_CTRL_S *pstCtrl);
BS_STATUS _NAT_UDP_SetPubIp
(
    IN _NAT_UDP_CTRL_S *pstCtrl,
    IN UINT auiPubIp[NAT_MAX_PUB_IP_NUM] 
);
BS_STATUS _NAT_UDP_PktIn
(
    IN _NAT_UDP_CTRL_S *pstCtrl,
    IN IP_HEAD_S  *pstIpHead,
    IN UCHAR *pucData,
    IN UINT uiDataLen,
    IN BOOL_T bFromPub,
    INOUT UINT *puiDomainId
);
VOID _NAT_UDP_TimerStep(IN _NAT_UDP_CTRL_S *pstCtrl);
VOID _NAT_UDP_Walk
(
    IN _NAT_UDP_CTRL_S *pstCtrl,
    IN PF_NAT_WALK_CALL_BACK_FUNC pfFunc,
    IN HANDLE hUserHandle
);


#endif

#ifdef __cplusplus
    }
#endif 

#endif 


