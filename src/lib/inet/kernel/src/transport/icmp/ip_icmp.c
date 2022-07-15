/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-11-24
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mbuf_utl.h"


int badport_bandlim(int which)
{
    /* 不进行限速 */
    return 0;
}


/* 发送ICMP差错报文接口 */
void icmp_error(IN MBUF_S *n, int type, int code, UINT dest, int mtu)
{
    /* TODO :  */
    return;
}
