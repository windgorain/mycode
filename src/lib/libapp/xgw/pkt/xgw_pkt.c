/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0
* Description:
******************************************************************************/
#include "bs.h"
#include "utl/ip_utl.h"
#include "utl/endian_utl.h"
#include "utl/vxlan_utl.h"
#include "utl/in_checksum.h"
#include "../h/xgw_vht.h"
#include "../h/xgw_vrt.h"

enum {
    XGW_RET_ACCEPT = 0,
    XGW_RET_DROP
};

static VXLAN_HEAD_S * _xgw_pkt_get_vxlan_header(IP_HEAD_S *ip, int buf_len)
{
    VXLAN_HEAD_S *vxlan_hdr;
    if (ip->ucProto != IP_PROTO_UDP) {
        return NULL;
    }

    vxlan_hdr = VXLAN_GetVxlanHeader(ip, buf_len, NET_PKT_TYPE_IP);
    if (! vxlan_hdr) {
        return NULL;
    }

    if (! VXLAN_Valid(vxlan_hdr)) {
        return NULL;
    }

    return vxlan_hdr;
}

static int _xgw_pkt_get_route(U32 vid, U32 dip, OUT U32 *nvid, OUT U32 *hip)
{
    U64 nexthop = XGW_VRT_Match(ntoh3B(vid), ntohl(dip));
    if (! nexthop) {
        RETURN(BS_NOT_FOUND);
    }

    U32 vni = nexthop >> 32;
    U32 nip = nexthop;

    if (! nip) {
        nip = XGW_VHT_GetHip(vni, dip);
        if (! nip) {
            RETURN(BS_NOT_FOUND);
        }
    }

    *nvid = vni;
    *hip = nip;

    return 0;
}

static int _xgw_pkt_get_info(IP_HEAD_S *ip, int buf_len, OUT VXLAN_HEAD_S **vxlan_hdr, OUT IP_HEAD_S **inner_ip_hdr)
{
    int reserved_len;
    VXLAN_HEAD_S *vxlanhdr;
    IP_HEAD_S *inner_iphdr;

    vxlanhdr = _xgw_pkt_get_vxlan_header(ip, buf_len);
    if (! vxlanhdr) {
        return -1;
    }

    reserved_len = buf_len - ((char*)vxlanhdr - (char*)ip);

    inner_iphdr = VXLAN_GetInnerIPPkt(vxlanhdr, reserved_len, FALSE);
    if (! inner_iphdr) {
        return -1;
    }

    *vxlan_hdr = vxlanhdr;
    *inner_ip_hdr = inner_iphdr;

    return 0;
}

static int _xgw_vgw_pkt_process(IP_HEAD_S *ip, int buf_len, void *pkt)
{
    VXLAN_HEAD_S *vxlan_hdr;
    IP_HEAD_S *inner_ip_hdr;
    U32 nvid, hip;

    if (_xgw_pkt_get_info(ip, buf_len, &vxlan_hdr, &inner_ip_hdr) < 0) {
        return XGW_RET_ACCEPT;
    }

    if (_xgw_pkt_get_route(vxlan_hdr->vni, inner_ip_hdr->unDstIp.uiIp, &nvid, &hip) < 0) {
        return XGW_RET_DROP;
    }

    ip->usCrc = IN_CHKSUM_Change((void*)&ip->unDstIp.uiIp, 2, (void*)&hip, 2, ip->usCrc);
    ip->unDstIp.uiIp = hip;
    vxlan_hdr->vni = nvid;

    return XGW_RET_ACCEPT;
}

int XGW_PKT_Process(IP_HEAD_S *ip, int buf_len, void *pkt)
{
    return _xgw_vgw_pkt_process(ip, buf_len, pkt);
}

