/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _IP_DEF_H
#define _IP_DEF_H

#include "utl/csum_utl.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define IP_HEADER_SIZE 20
#define TCP_HEADER_SIZE 20
#define UDP_HEADER_SIZE 8

/* 填充IP头, sip和dip都是网络序 */
static inline void IP_FillIpHeader(UINT sip, UINT dip, UCHAR protocol, USHORT total_len, OUT struct iphdr *iph)
{
    iph->saddr = sip;
    iph->daddr = dip;
    iph->version = 4;
    iph->ihl = sizeof(struct iphdr) / 4;
    iph->tos = 0;
    iph->tot_len = htons(total_len);
    iph->id = 0xabcd;
    iph->frag_off = 0;
    iph->protocol = protocol;
    iph->ttl = 0xff;
    iph->check = 0;
}

#ifdef __cplusplus
}
#endif
#endif //IP_DEF_H_
