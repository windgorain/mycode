/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-8-24
* Description: 
* History:     
******************************************************************************/

#ifndef __TP_INNER_H_
#define __TP_INNER_H_

#include "utl/vclock_utl.h"
#include "utl/ka_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif 

#define _TP_PROTO_MAX  0xffff  
#define _TP_ID_MAX     0xffff  

#define _TP_HASH_BUCKET 1024

#define _TP_PROTOCOL_MAX_ACCEPT_NUM 32    


typedef enum
{
    _TP_STATUS_INIT = 0,
    _TP_STATUS_SYN_SEND,
    _TP_STATUS_SYN_RCVD,
    _TP_STATUS_ESTABLISH,
    _TP_STATUS_FIN_WAIT_1,
    _TP_STATUS_FIN_WAIT_2,
    _TP_STATUS_CLOSING,
    _TP_STATUS_TIME_WAIT,
    _TP_STATUS_CLOSE_WAIT,
    _TP_STATUS_LAST_ACK,
    _TP_STATUS_CLOSED,

    _TP_STATUS_MAX
}_TP_STATUS_E;


#define _TP_SOCKET_FLAG_NBIO      0x1
#define _TP_SOCKET_FLAG_ACCEPTING 0x2

typedef struct
{
    USHORT usIdle;           
    USHORT usIntval;         
    USHORT usProbeMaxCount;  
    USHORT usProbeCount;     
    VCLOCK_HANDLE hKeepAliveTimer;
}_TP_KEEP_ALIVE_S;

typedef struct
{
    USHORT usResendCount;   
    VCLOCK_HANDLE hResendTImer;
}_TP_RESEND_S;

typedef struct
{
    TP_TYPE_E eType;
    UINT uiFlag;         
    UINT uiProtocolId;   
    TP_ID uiListenTpId;  
    TP_ID uiLocalTpId;   
    TP_ID uiPeerTpId;    
    UINT uiStatus;       
    INT iSn;             
    INT iAckSn;          
    TP_CHANNEL_S stChannel;     
    MBUF_QUE_S stSendMbufQue;   
    MBUF_QUE_S stRecvMbufQue;   
    EVENT_HANDLE hEvent;
    KA_S stKeepAlive;   
    _TP_RESEND_S stResend;
    HANDLE *phPropertys;    
}_TP_SOCKET_S;


#define _TP_PROTOCOL_FLAG_LISTEN 0x1 

typedef struct
{
    TP_ID uiLocalTpId;
    UINT uiProtocolId;  
    UINT uiFlag;
    UINT uiAccepttingNum;
    _TP_SOCKET_S * astAcceptting[_TP_PROTOCOL_MAX_ACCEPT_NUM]; 
}_TP_PROTOCOL_S;

typedef struct
{
    HANDLE hSocketNap;
    HANDLE hProtocolNap;
    VCLOCK_INSTANCE_HANDLE hVclockHandle;
    PF_TP_SEND_FUNC pfSendFunc;
    PF_TP_EVENT_FUNC pfEventFunc;
    USER_HANDLE_S stUserHandle;
    UINT uiMaxPropertys;
    UINT uiDbgFlag;

    USHORT usDftIdle;   
    USHORT usDftIntval; 
    USHORT usDftMaxProbeCount; 
    
    DLL_HEAD_S stCloseNotifyList;
}_TP_CTRL_S;

typedef VOID (*PF_TP_SOCKET_WALK_FUNC)(IN _TP_CTRL_S *pstCtrl, IN _TP_SOCKET_S *pstSocket, IN HANDLE hUserHandle);

BS_STATUS _TP_Socket_Init(_TP_CTRL_S *pstCtrl);
VOID _TP_Socket_UnInit(_TP_CTRL_S *pstCtrl);
_TP_SOCKET_S * _TP_Socket_NewSocket
(
    IN _TP_CTRL_S *pstCtrl,
    IN TP_TYPE_E eType
);
VOID _TP_Socket_FreeSocket(IN _TP_CTRL_S *pstCtrl, IN _TP_SOCKET_S *pstTpSocket);
_TP_SOCKET_S * _TP_Socket_FindSocket(IN _TP_CTRL_S *pstCtrl, IN TP_ID uiTpId);
BS_STATUS _TP_Socket_SetOpt
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_SOCKET_S *pstTpSocket,
    IN TP_OPT_E enOpt,
    IN VOID *pValue
);
BS_STATUS _TP_Protocol_Init(IN _TP_CTRL_S *pstCtrl);
VOID _TP_Protocol_UnInit(IN _TP_CTRL_S *pstCtrl);
_TP_PROTOCOL_S * _TP_Protocol_FindProtocol(IN _TP_CTRL_S *pstCtrl, IN UINT uiProtocolId);
BS_STATUS _TP_Protocol_BindProtocol
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_SOCKET_S *pstTpSocket,
    IN UINT uiProtocolId
);
BS_STATUS _TP_Protocol_UnBindProtocol
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_SOCKET_S *pstTpSocket
);
BS_STATUS _TP_Protocol_Listen(IN _TP_CTRL_S *pstCtrl, IN _TP_SOCKET_S *pstTpSocket);
BS_STATUS _TP_Protocol_Accept
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_SOCKET_S *pstTpSocket,
    OUT TP_ID *puiAcceptTpId
);
BOOL_T _TP_Protocol_CanAccepting
(
    IN _TP_PROTOCOL_S *pstProtocol
);
BS_STATUS _TP_Protocol_AddAccepting
(
    IN _TP_PROTOCOL_S *pstProtocol,
    IN _TP_SOCKET_S *pstTpSocket
);
VOID _TP_Protocol_DelAccepting
(
    IN _TP_PROTOCOL_S *pstProtocol,
    IN _TP_SOCKET_S *pstTpSocket
);
VOID _TP_PKT_InitKeepAlive
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_SOCKET_S *pstTpSocket
);
BS_STATUS _TP_PKT_SendSyn
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_SOCKET_S *pstTpSocket
);
BS_STATUS _TP_PKT_SendData
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_SOCKET_S *pstTpSocket,
    IN MBUF_S *pstMbuf
);
VOID _TP_PKT_ResetKeepAlive
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_SOCKET_S *pstTpSocket
);
VOID _TP_Socket_WakeUp
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_SOCKET_S *pstTpSocket,
    IN UINT uiEvent
);
VOID _TP_Socket_Abort(IN _TP_CTRL_S *pstCtrl, IN _TP_SOCKET_S *pstTpSocket);
BS_STATUS _TP_Socket_Walk
(
    IN _TP_CTRL_S *pstCtrl,
    IN PF_TP_SOCKET_WALK_FUNC pfFunc,
    IN HANDLE hUserHandle
);

#ifdef __cplusplus
    }
#endif 

#endif 


