/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2014-2-25
* Description: 
* History:     
******************************************************************************/

#ifndef __VS_SOCKET_H_
#define __VS_SOCKET_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

/*
 * Socket state bits.
 *
 * Historically, this bits were all kept in the so_state field.  For
 * locking reasons, they are now in multiple fields, as they are
 * locked differently.  so_state maintains basic socket state protected
 * by the socket lock.  so_qstate holds information about the socket
 * accept queues.  Each socket buffer also has a state field holding
 * information relevant to that socket buffer (can't send, rcv).  Many
 * fields will be read without locks to improve performance and avoid
 * lock order issues.  However, this approach must be used with caution.
 */
#define SS_NOFDREF         0x0001    /* no file table ref any more */
#define SS_ISCONNECTED     0x0002    /* socket connected to a peer */
#define SS_ISCONNECTING    0x0004    /* in process of connecting to peer */
#define SS_ISDISCONNECTING 0x0008    /* in process of disconnecting */
#define SS_NBIO            0x0100    /* non-blocking ops(in Leopard,this won't be used) */
#define SS_ISCONFIRMING    0x0400    /* deciding to accept connection req */
#define SS_CANBIND         0x1000    /* can bind */
#define SS_ISDISCONNECTED  0x2000    /* socket disconnected from peer */
#define SS_PROTOREF        0x4000    /* strong protocol reference */


typedef struct sockbuf
{

}VS_SOCKBUF_S;

typedef struct
{
    USHORT usType;
    USHORT usState;
    UINT   uiOptions;
    VS_FD_S *pstFile;
    VS_PROTOSW_S *pstProto;
    DLL_HEAD_S stCompList;
    DLL_HEAD_S stInCompList;
    VS_SOCKBUF_S stSndBuf;
    VS_SOCKBUF_S stRcvBuf;
}VS_SOCKET_S;


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VS_SOCKET_H_*/


