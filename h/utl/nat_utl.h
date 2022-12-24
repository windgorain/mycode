
#ifndef __NAT_H_
#define __NAT_H_

#include "utl/mbuf_utl.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define NAT_MAX_PUB_IP_NUM 16 /* 最多支持的对外公网IP的个数 */

#define NAT_DBG_PACKET 0x1

typedef HANDLE NAT_HANDLE;

/* NAT TCP 映射表的状态 */
typedef enum
{
    NAT_TCP_STATUS_SYN_SEND = 0,    /* 收到内网的SYN报文后,新建表项的状态 */
    NAT_TCP_STATUS_SYN_RECEIVED,    /* 收到外网的SYN+ACK */
    NAT_TCP_STATUS_ESTABLISHED,     /* 收到内网的ACK */

    NAT_TCP_STATUS_I_FIN_WAIT_1,      /* 收到内网的FIN */
    NAT_TCP_STATUS_I_FIN_WAIT_2,      /* 收到外网的对FIN的ACK */
    NAT_TCP_STATUS_I_TIME_WAIT,       /* 收到外网的FIN */
                                      /* 收到内网对FIN的ACK，直接跳到TIME_WAIT */

    NAT_TCP_STATUS_O_FIN_WAIT_1,      /* 收到外网的FIN */
    NAT_TCP_STATUS_O_FIN_WAIT_2,      /* 收到内网的对FIN的ACK */
    NAT_TCP_STATUS_O_TIME_WAIT,       /* 收到内网的FIN */
                                      /* 收到外网对FIN的ACK，直接跳到TIME_WAIT */

    NAT_TCP_STATUS_TIME_WAIT,         /* 处于TIME_WAIT状态时,收到对端的ACK */

    NAT_TCP_STATUS_CLOSED,

    NAT_TCP_STATUS_MAX
}NAT_TCP_STATUS_E;

/* NAT UDP 映射表的状态 */
typedef enum
{
    NAT_UDP_STATUS_SYN_SEND = 0,    /* 收到内网的一个UDP报文后,新建表项的状态 */
    NAT_UDP_STATUS_SYN_RECEIVED,    /* 收到外网的一个UDP报文后 */
    NAT_UDP_STATUS_ESTABLISHED,     /* 再收到内网的一个UDP报文后 */

    NAT_UDP_STATUS_MAX
}NAT_UDP_STATUS_E;

typedef struct
{
    UINT uiPrivateIp;       /* 网络序 */
    UINT uiPubIp;           /* 网络序 */
    USHORT usPrivatePort;   /* 网络序 */
    USHORT usPubPort;       /* 网络序 */
    UINT uiDomainId;
    UCHAR ucType;
    UCHAR ucStatus;         /* NAT_TCP_STATUS_E */
    UCHAR aucReserved[2];
}NAT_NODE_S;

typedef BS_WALK_RET_E (*PF_NAT_WALK_CALL_BACK_FUNC)(IN NAT_NODE_S *pstNatNode, IN HANDLE hUserHandle);

NAT_HANDLE NAT_Create
(
    IN USHORT usMinPort,   /* 主机序 ,对外可转换的端口号最小值 */
    IN USHORT usMaxPort,    /* 主机序 ,对外可转换的端口号最大值 */
    IN UINT   uiMsInTick,  /* 多少ms为一个Tick */
    IN BOOL_T bCreateMutex
);

VOID NAT_Destory(IN NAT_HANDLE hNatHandle);

VOID NAT_SetPubIp
(
    IN NAT_HANDLE hNatHandle,
    IN UINT auiPubIp[NAT_MAX_PUB_IP_NUM] /* 网络序，提供的对外公网IP */
);

BS_STATUS NAT_SetTcpTimeOutTick
(
    IN NAT_HANDLE hNatHandle,
    IN UINT uiTick
);

BS_STATUS NAT_SetUdpTimeOutTick
(
    IN NAT_HANDLE hNatHandle,
    IN UINT uiTick
);

BS_STATUS NAT_PacketTranslate
(
    IN NAT_HANDLE hNatHandle,
    INOUT UCHAR *pucData, /* IP报文 */
    IN UINT uiDataLen,
    IN BOOL_T bFromPub,
    INOUT UINT *puiDomainId   /* 私有报文为IN, Pub报文为OUT */
);

/* 调用者负责释放Mbuf，此函数不释放 */
BS_STATUS NAT_PacketTranslateByMbuf
(
    IN NAT_HANDLE hNatHandle,
    INOUT MBUF_S *pstMbuf, /* IP报文 */
    IN BOOL_T bFromPub,
    INOUT UINT *puiDomainId   /* 私有报文为IN, Pub报文为OUT */
);

VOID NAT_TimerStep(IN NAT_HANDLE hNatHandle);

VOID NAT_Walk
(
    IN NAT_HANDLE hNatHandle,
    IN PF_NAT_WALK_CALL_BACK_FUNC pfFunc,
    IN HANDLE hUserHandle
);

VOID NAT_SetDbgFlag(IN NAT_HANDLE hNatHandle, IN UINT uiDbgFlag);

VOID NAT_ClrDbgFlag(IN NAT_HANDLE hNatHandle, IN UINT uiDbgFlag);

CHAR * NAT_GetStatusString(IN UCHAR ucType, IN UINT uiStatus);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*__NAT_H_*/


