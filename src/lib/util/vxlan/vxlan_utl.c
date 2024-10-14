/*================================================================
*   Created by LiXingang
*   Author: Xingang.Li  Version: 1.0  Date: 2017-10-4
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/net.h"
#include "utl/eth_utl.h"
#include "utl/ip_utl.h"
#include "utl/udp_utl.h"
#include "utl/vxlan_utl.h"

BOOL_T VXLAN_Valid(IN VXLAN_HEAD_S *vxlan_header)
{

    if ((vxlan_header->flag & VXLAN_FLAG_I) == 0) {
        return FALSE;
    }

    if (VXLAN_GET_VER(vxlan_header->flag) != 0) {
        return FALSE;
    }

    return TRUE;
}

VXLAN_HEAD_S * VXLAN_GetVxlanHeader(IN void *pkt_buf, int buf_len, NET_PKT_TYPE_E pkt_type)
{
    UDP_HEAD_S *udp_header;
    UINT head_len = 0;

    if (pkt_type != NET_PKT_TYPE_VXLAN) {
        udp_header = UDP_GetUDPHeader(pkt_buf, buf_len, pkt_type);
        if (udp_header == NULL) {
            return NULL;
        }

        head_len = ((UCHAR*)udp_header - (UCHAR*)pkt_buf)
            + sizeof(UDP_HEAD_S);
    }

    if ((buf_len - head_len) < sizeof(VXLAN_HEAD_S)) {
        return NULL;
    }

    return (void*)((char*)pkt_buf + head_len);
}

int VXLAN_GetInnerPktType(IN VXLAN_HEAD_S *vxlan_header, int is_ip_vxlan)
{
    if (vxlan_header->flag & VXLAN_FLAG_P) {
        return vxlan_header->next_protocol;
    }

    if (is_ip_vxlan) {
        return VXLAN_NEXT_PROTOCOL_IPv4;
    }

    return VXLAN_NEXT_PROTOCOL_ETH;
}


void * VXLAN_GetInnerIPPkt(VXLAN_HEAD_S *vxlan_header, int buf_len, int is_ip_vxlan )
{
    int left_len;
    int inner_pkt_type;
    void *data= NULL;
    IP46_HEAD_S ip_header;
    int ret = BS_ERR;

    data = (void*)(vxlan_header + 1);
    left_len = buf_len - sizeof(VXLAN_HEAD_S);

    inner_pkt_type = VXLAN_GetInnerPktType(vxlan_header, is_ip_vxlan);
    if (inner_pkt_type == VXLAN_NEXT_PROTOCOL_ETH) {
        ret = IP46_GetIPHeader(&ip_header, data, left_len,  NET_PKT_TYPE_ETH); 
    } else if (inner_pkt_type == VXLAN_NEXT_PROTOCOL_IPv4) {
        ret = IP46_GetIPHeader(&ip_header, data, left_len,  NET_PKT_TYPE_IP); 
    }

    if (ret != 0) {
        return NULL;
    }

    if (ip_header.family != ETH_P_IP) {
        return NULL;
    }

    return ip_header.iph.ip4;
}

static char * _vxlan_GetFlagsString(IN UCHAR ucFlags, char *info)
{
    char *ptr = info;

    if (ucFlags & VXLAN_FLAG_I) {
        *ptr++ = 'I';
    }
    if (ucFlags & VXLAN_FLAG_P) {
        *ptr++ = 'P';
    }
    if (ucFlags & VXLAN_FLAG_B) {
        *ptr++ = 'B';
    }
    if (ucFlags & VXLAN_FLAG_O) {
        *ptr++ = 'O';
    }

    *ptr = '\0';

    return info;
}

char* VXLAN_Header2String(VXLAN_HEAD_S *vxlan_header,
        OUT CHAR *info, UINT infosize)
{
    char szFlags[16];

    snprintf(info, infosize,
            "\"flag\":\"%s\",\"next_protocol\":%u,\"vni\":%u",
            _vxlan_GetFlagsString(vxlan_header->flag, szFlags),
            vxlan_header->next_protocol, vxlan_header->vni);

    return info;
}
