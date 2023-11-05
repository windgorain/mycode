/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-11-15
* Description: 
* History:     
******************************************************************************/

#ifndef __UDP_FUNC_H_
#define __UDP_FUNC_H_

#ifdef __cplusplus
    extern "C" {
#endif 

VOID udp_Init();

int udp_servinit(void);

void udp_slowtimo(void);

VOID udp_input(IN MBUF_S *pstMbuf, IN UINT uiPayloadOffset);

void udp_ctlinput
(
    IN int cmd,
    IN SOCKADDR_S *sa,
    IN void *vip
);

#ifdef __cplusplus
    }
#endif 

#endif 


