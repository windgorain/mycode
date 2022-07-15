/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2012-11-26
* Description: 
* History:     
******************************************************************************/

#ifndef __INET_IP_H_
#define __INET_IP_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

extern UCHAR inetctlerrmap[];

void ip_savecontrol
(
    IN INPCB_S *inp,
    OUT MBUF_S **mp,
    IN IP_HEAD_S *ip,
    IN MBUF_S *m,
    IN UCHAR *ip_header
);

void ip_setopt2mbuf
(
    IN INPCB_S *inp,
    IN MBUF_S *control,
    IN MBUF_S *m
);

int ip_ctloutput(IN SOCKET_S *so, IN SOCKOPT_S *sopt);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__INET_IP_H_*/


