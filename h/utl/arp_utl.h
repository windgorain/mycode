/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-9-30
* Description: 
* History:     
******************************************************************************/

#ifndef __ARP_UTL_H_
#define __ARP_UTL_H_

#include "utl/mbuf_utl.h"
#include "utl/eth_utl.h"
#include "utl/sif_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define ARP_HARDWARE_TYPE_ETH 0x0001

#define ARP_REQUEST   0x0001
#define ARP_REPLY     0x0002

typedef HANDLE ARP_HANDLE;

typedef enum
{
    ARP_TYPE_STATIC = 0,
    ARP_TYPE_DYNAMIC    
}ARP_TYPE_E;

#pragma pack(1)
typedef struct
{
    /* ETH头 */
    UCHAR  aucDMac[6];          /* Reverse these two */
    UCHAR  aucSMac[6];          /* to answer ARP requests*/
    USHORT usProto;             /* 0x0806 :  ETH_P_ARP */

    /* ARP头 */
    USHORT usHardWareType;      /* 硬件类型, 对于以太网是网络序的1:ARP_HARDWARE_TYPE_ETH */
    USHORT usProtoType;         /* 高层协议地址类型, 对于IP 地址为 0x0800:  ETH_P_IP */
    UCHAR  ucHardAddrLen;       /* 硬件地址长度, 对于以太网为 0x06 */
    UCHAR  ucProtoAddrLen;      /* 协议地址长度, 对于IP地址为 0x04 */
    USHORT usOperation;         /* 操作类型,  ARP_REQUEST for ARP request, ARP_REPLY for ARP reply */
    UCHAR  aucSenderHA[6];      /* 发送方硬件地址 */
    UINT  ulSenderIp;          /* 发送方IP地址 */
    UCHAR  aucDstHA[6];         /* 目标硬件地址 */
    UINT  ulDstIp;             /* 目标IP地址 */
}ETH_ARP_PACKET_S;

typedef struct
{
    USHORT usHardWareType;      /* 硬件类型, 对于以太网是网络序的1:ARP_HARDWARE_TYPE_ETH */
    USHORT usProtoType;         /* 高层协议地址类型, 对于IP 地址为 0x0800:  ETH_P_IP */
    UCHAR  ucHardAddrLen;       /* 硬件地址长度, 对于以太网为 0x06 */
    UCHAR  ucProtoAddrLen;      /* 协议地址长度, 对于IP地址为 0x04 */
    USHORT usOperation;         /* 操作类型,  ARP_REQUEST for ARP request, ARP_REPLY for ARP reply */
    UCHAR  aucSenderHA[6];      /* 发送方硬件地址 */
    UINT  ulSenderIp;          /* 发送方IP地址 */
    UCHAR  aucDstHA[6];         /* 目标硬件地址 */
    UINT  ulDstIp;             /* 目标IP地址 */
}ARP_HEADER_S;

#pragma pack()

typedef struct
{
    UINT uiIp;  /* 网络序 */
    MAC_ADDR_S stMac;
    ARP_TYPE_E eType;
}ARP_NODE_S;

typedef BS_STATUS (*PF_ARP_SEND_PACKET_FUNC)(IN MBUF_S *pstMbuf, IN USER_HANDLE_S *pstUserHandle);
typedef BOOL_T (*PF_ARP_IS_HOST_IP_FUNC)(IF_INDEX uiIfIndex, ARP_HEADER_S *pstArpHeader, USER_HANDLE_S *pstUserHandle);
typedef UINT (*PF_ARP_GET_HOST_IP_FUNC)(IN IF_INDEX ifIndex, IN UINT uiDstIP/* 网络序 */, IN USER_HANDLE_S *pstUserHandle);  /* 返回网络序IP */
typedef BS_WALK_RET_E (*PF_ARP_WALK_CALL_BACK_FUNC)(IN ARP_NODE_S *pstArpNode, IN HANDLE hUserHandle);


BOOL_T ARP_IsArpPacket(IN MBUF_S *pstMbuf);

MBUF_S * ARP_BuildPacket
(
    IN UINT uiSrcIp,
    IN UINT uiDstIp, 
    IN UCHAR *pucSrcMac,
    IN UCHAR *pucDstMac,  /* 如果为NULL, 则表示要广播 */
    IN USHORT usArpType
);

MBUF_S * ARP_BuildPacketWithEthHeader
(
    IN UINT uiSrcIp,  /* 网络序 */
    IN UINT uiDstIp, /* 网络序 */
    IN UCHAR *pucSrcMac,
    IN UCHAR *pucDstMac,
    IN USHORT usArpType
);

ARP_HANDLE ARP_CreateInstance
(
    IN UINT uiTimeOutTick,  /* ARP表项超时时间, 单位tick */
    IN BOOL_T bIsCreateSem
);

VOID ARP_DestoryInstance(IN ARP_HANDLE hArpInstance);
VOID ARP_SetSendPacketFunc
(
    IN ARP_HANDLE hArpInstance,
    IN PF_ARP_SEND_PACKET_FUNC pfSendPacketFunc,
    IN USER_HANDLE_S *pstUserHandle
);
VOID ARP_SetIsHostIpFunc
(
    IN ARP_HANDLE hArpInstance,
    IN PF_ARP_IS_HOST_IP_FUNC pfIsHostIp,
    IN USER_HANDLE_S *pstUserHandle
);
VOID ARP_SetGetHostIpFunc
(
    IN ARP_HANDLE hArpInstance,
    IN PF_ARP_GET_HOST_IP_FUNC pfGetHostIp,
    IN USER_HANDLE_S *pstUserHandle
);
BS_STATUS ARP_SetHostMac(IN ARP_HANDLE hArpInstance, IN MAC_ADDR_S *pstHostMac);
BS_STATUS ARP_AddStaticARP
(
    IN ARP_HANDLE hArpInstance,
    IN UINT uiIP, /* 网络序 */
    IN MAC_ADDR_S *pstMac
);
/* 根据IP得到MAC，如果得不到,则发送ARP请求，并返回BS_AGAIN. */
BS_STATUS ARP_GetMacByIp
(
    IN ARP_HANDLE hArpInstance,
    IN UINT uiIfIndex,
    IN UINT ulIpToResolve /* 网络序 */,
    IN MBUF_S *pstMbuf,
    OUT MAC_ADDR_S *pstMacAddr
);

VOID ARP_Walk
(
    IN ARP_HANDLE hArpInstance,
    IN PF_ARP_WALK_CALL_BACK_FUNC pfFunc,
    IN HANDLE hUserHandle
);

BS_STATUS ARP_PacketInput(IN ARP_HANDLE hArpInstance, IN MBUF_S *pstArpPacket);

VOID ARP_TimerStep(IN ARP_HANDLE hArpInstance);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__ARP_UTL_H_*/


