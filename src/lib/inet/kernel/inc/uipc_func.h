/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-12-16
* Description: 
* History:     
******************************************************************************/

#ifndef __UIPC_FUNC_H_
#define __UIPC_FUNC_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

int sbappendaddr_locked
(
    IN SOCKBUF_S *sb,
    IN SOCKADDR_S *asa,
    IN MBUF_S *m0,
    IN MBUF_S *control
);
void sowakeup(IN struct socket *so, IN SOCKBUF_S *sb);

MBUF_S * sbcreatecontrol(IN VOID * p, IN int size, IN int type, IN int level);

int soreserve(IN SOCKET_S *so, IN UINT sndcc, IN UINT rcvcc);

void soisconnected(IN SOCKET_S *so);

int sosend_dgram
(
    IN SOCKET_S *so,
    IN SOCKADDR_S *addr,
    IN UIO_S *uio,
    IN MBUF_S *top, 
    IN MBUF_S *control,
    IN int flags
);

void soisdisconnected(IN SOCKET_S *so);

void socantsendmore(IN SOCKET_S *so);

void sbdrop_locked(IN SOCKBUF_S *sb, IN int len);

void sbdrop(IN SOCKBUF_S *sb, IN int len);

int sooptcopyin(IN struct sockopt *sopt, OUT void *buf, IN UINT len, IN UINT minlen);

int sooptcopyout(OUT struct sockopt *sopt, IN const void *buf, IN UINT len);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__UIPC_FUNC_H_*/


