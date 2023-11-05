/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-11-21
* Description: 
* History:     
******************************************************************************/

#ifndef __UIPC_SOCK_FUNC_H_
#define __UIPC_SOCK_FUNC_H_

#ifdef __cplusplus
    extern "C" {
#endif 

int so_msleep(IN SEM_HANDLE hSemToP, IN SEM_HANDLE hSemToV, IN int timeo);
int sbreserve(IN SOCKBUF_S *sb, IN UINT cc, IN SOCKET_S *so);
VOID sbdestroy(IN SOCKBUF_S *sb, IN SOCKET_S *so);
void sbrelease_locked(IN SOCKBUF_S *sb, IN SOCKET_S *so);
int sbreserve_locked(IN SOCKBUF_S *sb, IN UINT cc, IN SOCKET_S *so);
int sbreserve(IN SOCKBUF_S *sb, IN UINT cc, IN SOCKET_S *so);

SOCKET_S * socreate(IN UINT uiDomain, IN USHORT usType, IN USHORT usProtocol);
int soclose(IN SOCKET_S *so);
int sobind(SOCKET_S *so, struct sockaddr *nam);
int solisten(IN SOCKET_S *so, IN int iBackLog);
int soaccept(IN SOCKET_S *so, OUT struct sockaddr **ppNam);
int soconnect(IN SOCKET_S *so, SOCKADDR_S *nam);
int sodisconnect(IN SOCKET_S *so);
int sosend
(
    IN SOCKET_S *so,
    IN SOCKADDR_S *addr,
    IN UIO_S *uio,
    IN MBUF_S *top,
    IN MBUF_S *control,
    IN int flags
);
int soreceive
(
    IN SOCKET_S *so,
    OUT SOCKADDR_S **psa,
    OUT UIO_S *uio,
    OUT MBUF_S **mp0,
    OUT MBUF_S **controlp,
    INOUT int *flagsp
);
int sosetopt(IN SOCKET_S *so, IN SOCKOPT_S *sopt);
int sogetopt(IN SOCKET_S *so, INOUT SOCKOPT_S *sopt);
int sogetsockname(IN SOCKET_S *so, OUT SOCKADDR_S **ppstSa);
int sogetpeername(IN SOCKET_S *so, OUT SOCKADDR_S **ppstSa);


#ifdef __cplusplus
    }
#endif 

#endif 


