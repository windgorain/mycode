/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-7-15
* Description: WAN的协议层注册分发器
* History:     
******************************************************************************/
#include "bs.h"
    
#include "utl/bit_opt.h"
#include "utl/sif_utl.h"
#include "utl/ip_utl.h"
#include "utl/eth_utl.h"
#include "app/wan_pub.h"
#include "app/if_pub.h"

#include "../h/wan_ipfwd.h"
#include "../h/wan_arp.h"
#include "../h/wan_proto.h"


PF_WAN_PROTO_INPUT g_apfWanProtoTbl[65536];     /* 以网络序的协议号为下标 */


BS_STATUS WAN_Proto_Input(IN IF_INDEX ifIndex, IN MBUF_S *pstMbuf, IN USHORT usProtoType/* 网络序 */)
{
    PF_WAN_PROTO_INPUT pfFunc;

    MBUF_SET_RECV_IF_INDEX(pstMbuf, ifIndex);

    pfFunc = g_apfWanProtoTbl[usProtoType];
    if (pfFunc != NULL)
    {
        return pfFunc(pstMbuf);
    }
    else
    {
        MBUF_Free(pstMbuf);
        return (BS_ERR);
    }
}

BS_STATUS WAN_Proto_Init()
{
    Mem_Zero(g_apfWanProtoTbl, sizeof(g_apfWanProtoTbl));

    g_apfWanProtoTbl[htons(ETH_P_IP)] = WAN_IpFwd_Input;
    g_apfWanProtoTbl[htons(ETH_P_ARP)] = WAN_ARP_PacketInput;

    return BS_OK;
}

BS_STATUS WAN_Proto_RegProto(IN USHORT usProtoType/* 网络序 */,  IN PF_WAN_PROTO_INPUT pfFunc)
{
    g_apfWanProtoTbl[usProtoType] = pfFunc;

	return BS_OK;
}


