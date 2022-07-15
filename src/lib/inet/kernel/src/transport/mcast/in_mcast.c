/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-11-25
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mbuf_utl.h"
#include "utl/ip_utl.h"
#include "utl/in_checksum.h"
#include "inet/in_pub.h"

#include "protosw.h"
#include "domain.h"
#include "in.h"
#include "in_ipaddr.h"
#include "udp_func.h"
#include "ip_options.h"
#include "uipc_func.h"
#include "uipc_compat.h"

#include "../in_pcb.h"
#include "../in_mcast.h"
#include "../icmp.h"
#include "../inet_ip.h"
#include "../inet6_ip.h"

/* 查找匹配的组播group entry。匹配的原则是: 组播地址必须相等，如果
传入的ifindex不为IF_INVALID_INDEX，则必须匹配ifindex。 */
UINT imo_match_group(IN IP_MOPTIONS_S *imo, IN UINT ifindex, IN SOCKADDR_S *group)
{
    return (UINT)(-1);
}

/* 匹配源过滤地址 */
IN_MSOURCE_S * imo_match_source(IN IP_MOPTIONS_S *imo, IN UINT gidx, IN SOCKADDR_S *src)
{
    return NULL;
}

/* 释放组播选项内容 */
void inp_freemoptions(IN IP_MOPTIONS_S *imo)
{
    return;
}

/* 组播选项的设置 */
int inp_setmoptions(IN INPCB_S *inp, IN SOCKOPT_S *sopt)
{
    return 0;
}

/* 本函数实现组播选项的获取 */
int inp_getmoptions(struct inpcb *inp, struct sockopt *sopt)
{
    return ENOTSOCK;
}



