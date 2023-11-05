#ifndef __TP_UTL_H_
#define __TP_UTL_H_

#include "utl/mbuf_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef HANDLE TP_HANDLE;
typedef UINT TP_ID;

typedef struct
{
    HANDLE ahChannelDesc[4];
}TP_CHANNEL_S;

typedef enum
{
    TP_TYPE_PROTOCOL,   
    TP_TYPE_CONN,       

    TP_TYPE_MAX
}TP_TYPE_E;

typedef enum
{
    TP_OPT_NBIO = 0, 
    TP_OPT_KEEPALIVE_TIME, 

    TP_OPT_MAX
}TP_OPT_E;

typedef struct
{
    USHORT usIdle;           
    USHORT usIntval;         
    USHORT usMaxProbeCount;  
}TP_OPT_KEEP_ALIVE_S;

typedef struct
{
    TP_TYPE_E eType;
    UINT uiFlag;
    UINT uiProtocolId;   
    TP_ID uiLocalTpId;   
    TP_ID uiPeerTpId;    
    UINT uiStatus;       
    INT iSn;             
    INT iAckSn;          
    UINT uiSendBufCount;    
    UINT uiRecvBufCount;    
    TP_CHANNEL_S stChannel;
}TP_STATE_S;

typedef BS_STATUS (*PF_TP_SEND_FUNC)(IN TP_HANDLE hTpHandle, IN TP_CHANNEL_S *pstChannel, IN MBUF_S *pstMbuf, IN USER_HANDLE_S *pstUserHandle);

#define TP_EVENT_CONNECT 0x1
#define TP_EVENT_ACCEPT  0x2
#define TP_EVENT_READ    0x4
#define TP_EVENT_WRITE   0x8
#define TP_EVENT_ERR     0x10

typedef BS_STATUS (*PF_TP_EVENT_FUNC)(IN TP_HANDLE hTpHandle, IN TP_ID uiTpId, IN UINT uiEvent, IN USER_HANDLE_S *pstUserHandle);
typedef VOID (*PF_TP_WALK_FUNC)(IN TP_HANDLE hTpHandle, IN TP_ID uiTpId, IN HANDLE hUserHandle);

typedef VOID (*PF_TP_CLOSE_NOTIFY_FUNC)(IN UINT uiTPID, IN HANDLE *phPropertys, IN USER_HANDLE_S *pstUserHandle);

TP_HANDLE TP_Create
(
    IN PF_TP_SEND_FUNC pfSendFunc,
    IN PF_TP_EVENT_FUNC pfEventFunc,
    IN USER_HANDLE_S *pstUserHandle,
    IN UINT uiMaxPropertys 
);

BS_STATUS TP_SetDftKeepAlive
(
    IN TP_HANDLE hTpHandle,
    IN USHORT usIdle,
    IN USHORT usIntval,
    IN USHORT usMaxProbeCount
);

VOID TP_Destory(IN TP_HANDLE IN hTpHandle);

BS_STATUS TP_RegCloseNotifyEvent
(
    IN TP_HANDLE hTpHandle,
    IN PF_TP_CLOSE_NOTIFY_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
);

VOID TP_Walk(IN TP_HANDLE hTpHandle, IN PF_TP_WALK_FUNC pfFunc, IN HANDLE hUserHandle);

#define TP_DBG_FLAG_PKT 0x1

VOID TP_SetDbgFlag(IN TP_HANDLE hTpHandle, IN UINT uiDbgFlag);
VOID TP_ClrDbgFlag(IN TP_HANDLE hTpHandle, IN UINT uiDbgFlag);

TP_ID TP_Socket
(
    IN TP_HANDLE hTpHandle,
    IN TP_TYPE_E eType
);

BS_STATUS TP_SetOpt
(
    IN TP_HANDLE hTpHandle,
    IN TP_ID uiTpId,
    IN TP_OPT_E enOpt,
    IN VOID *pValue
);

BS_STATUS TP_SetProperty
(
    IN TP_HANDLE hTpHandle,
    IN TP_ID uiTpId,
    IN UINT uiIndex,
    IN HANDLE hValue
);

HANDLE TP_GetProperty
(
    IN TP_HANDLE hTpHandle,
    IN TP_ID uiTpId,
    IN UINT uiIndex
);

BS_STATUS TP_Bind
(
    IN TP_HANDLE hTpHandle,
    IN TP_ID uiTpId,
    IN UINT uiProtocolId 
);

BS_STATUS TP_Listen
(
    IN TP_HANDLE hTpHandle,
    IN TP_ID uiTpId
);

BS_STATUS TP_Close
(
    IN TP_HANDLE hTpHandle,
    IN TP_ID uiTpId
);

BS_STATUS TP_Connect
(
    IN TP_HANDLE hTpHandle,
    IN TP_ID uiTpId,
    IN UINT uiProtocolId,
    IN TP_CHANNEL_S *pstChannel
);

BS_STATUS TP_Accept
(
    IN TP_HANDLE hTpHandle,
    IN TP_ID uiTpId,
    OUT TP_ID *puiAcceptTpId
);

TP_CHANNEL_S * TP_GetChannel
(
    IN TP_HANDLE hTpHandle,
    IN TP_ID uiTpId
);

BS_STATUS TP_SendMbuf
(
    IN TP_HANDLE hTpHandle,
    IN TP_ID uiTpId,
    IN MBUF_S *pstMbuf
);

BS_STATUS TP_SendData
(
    IN TP_HANDLE hTpHandle,
    IN TP_ID uiTpId,
    IN UCHAR *pucData,
    IN UINT uiDataLen
);

MBUF_S * TP_RecvMbuf
(
    IN TP_HANDLE hTpHandle,
    IN TP_ID uiTpId
);

BS_STATUS TP_RecvData
(
    IN TP_HANDLE hTpHandle,
    IN TP_ID uiTpId,
    INOUT UCHAR *pucData,
    IN UINT uiMaxDataLen,
    OUT UINT *puiReadLen
);

BS_STATUS TP_PktInput
(
    IN TP_HANDLE hTpHandle,
    IN MBUF_S *pstMbuf,
    IN TP_CHANNEL_S *pstChannel
);

BOOL_T TP_IsSynPkt(IN MBUF_S *pstMbuf);

BOOL_T TP_NeedAck(IN MBUF_S *pstMbuf);

MBUF_S * TP_PktBuildRstByPkt(IN MBUF_S *pstMbuf);

BS_STATUS TP_TimerStep(IN TP_HANDLE hTpHandle);

BS_STATUS TP_GetState
(
    IN TP_HANDLE hTpHandle,
    IN TP_ID uiTpId,
    OUT TP_STATE_S *pstState
);

CHAR * TP_GetStatusString(IN UINT uiStatus);

VOID TP_TiggerKeepAlive(IN TP_HANDLE hTpHandle, IN TP_ID uiTpId);


#ifdef __cplusplus
    }
#endif 

#endif 


