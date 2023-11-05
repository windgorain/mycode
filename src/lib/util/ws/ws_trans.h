/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-7-6
* Description: 
* History:     
******************************************************************************/

#ifndef __WS_TRANS_H_
#define __WS_TRANS_H_

#include "utl/http_lib.h"
#include "utl/mime_utl.h"
#include "utl/fsm_utl.h"
#include "utl/kd_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif 

#define WS_TRANS_MAX_HEAD_LEN 2048  
#define WS_TRANS_MAX_BODY_BUF_LEN  2048  

#define WS_TRANS_FLAG_DROP_REQ_BODY    0x1  
#define WS_TRANS_FLAG_REPLY_AFTER_BODY 0x2  
#define WS_TRANS_FLAG_PAUSE            0x4  
#define WS_TRANS_FLAG_WILL_CONTINUE    0x8  
#define WS_TRANS_FLAG_ADD_REPLY_BODY_FINISHED  0x10  

typedef enum
{
    WS_STATE_RECV_HEAD = 0,
    WS_STATE_RECV_BODY,
    WS_STATE_RECV_BODY_OK,
    WS_STATE_PREPARE_HEAD,
    WS_STATE_BUILD_HEAD,
    WS_STATE_SEND_HEAD,
    WS_STATE_BUILD_BODY,
    WS_STATE_FORMAT_BODY,
    WS_STATE_SEND_BODY,
    WS_STATE_SEND_BODY_OK,
    WS_STATE_FINI,

    WS_STATE_MAX
}WS_STATE_E;

typedef struct
{
    UCHAR *pucData;
    UINT uiDataLen;
    UINT uiOffset;
}WS_TRANS_BUF_S;

typedef struct
{
    _WS_S *pstWs;
    WS_CONN_HANDLE hWsConn;
    MBUF_S *pstRecvMbuf;
    MBUF_S *pstSendMbuf;
    WS_TRANS_BUF_S stReplyHead;
    UINT uiContentLen;          
    UINT uiRemainContentLen;    
    HTTP_HEAD_PARSER hHttpHeadRequest;
    HTTP_HEAD_PARSER hHttpHeadReply;
    MIME_HANDLE hQuery;
    MIME_HANDLE hCookie;
    MIME_HANDLE hBodyMime;
    FSM_S stFsm;
    UINT uiFlag;  
    WS_VHOST_HANDLE hVHost;
    WS_CONTEXT_HANDLE hContext;
    CHAR *pcRequestFile;
    MEMPOOL_HANDLE hTransMemPool;
    VOID * apPlugContext[WS_PLUG_MAX];
    KD_HANDLE hKD;
}WS_TRANS_S;

BS_STATUS _WS_Trans_Init();
BS_STATUS _WS_Trans_EventInput(IN WS_CONN_HANDLE hWsConn, IN UINT uiEvent);
BS_STATUS _WS_Trans_RegFsmStateListener(IN PF_FSM_STATE_LISTEN pfListenFunc,IN USER_HANDLE_S * pstUserHandle);
CHAR * _WS_Trans_GetEventName(IN UINT uiEvent);
CHAR * _WS_Trans_GetStateName(IN UINT uiState);
VOID _WS_Trans_SetState(IN WS_TRANS_S *pstTrans, IN UINT uiState);
VOID _WS_Trans_SetFlag(IN WS_TRANS_S *pstTrans, IN UINT uiFlag);
MBUF_S * WS_Trans_GetBodyData(IN WS_TRANS_HANDLE hTrans);

#ifdef __cplusplus
    }
#endif 

#endif 


