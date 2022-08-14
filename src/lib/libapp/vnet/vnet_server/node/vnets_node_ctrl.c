/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-5-21
* Description: Node报文控制协议
* History:     
******************************************************************************/
#include "bs.h"

#include "comp/comp_if.h"

#include "../../inc/vnet_conf.h"
#include "../../inc/vnet_node.h"
    
#include "../inc/vnets_mac_layer.h"
#include "../inc/vnets_phy.h"
#include "../inc/vnets_ses.h"
#include "../inc/vnets_tp.h"
#include "../inc/vnets_domain.h"
#include "../inc/vnets_node.h"
#include "../inc/vnets_context.h"
#include "../inc/vnets_node_fwd.h"


static BS_STATUS vnets_nodectrl_Send(IN UINT uiDstSes, IN VOID *pData, IN UINT uiDataLen)
{
    MBUF_S *pstMbuf;

    pstMbuf = VNETS_Context_CreateMbufByCopyBuf(128, pData, uiDataLen);
    if (NULL == pstMbuf)
    {
        return BS_NO_MEMORY;
    }
    
    VNETS_Context_SetSendSesID(pstMbuf, uiDstSes);

    return IFNET_LinkOutput(VNETS_SES_GetIfIndex(uiDstSes), pstMbuf, 0);
}


BS_STATUS VNETS_NodeCtrl_ReplyOffline(IN UINT uiDstSes)
{
    VNET_NODE_PKT_HEADER_S stPkt = {0};
    UINT uiServerNid = VNET_NID_SERVER;

    stPkt.usFlag = htons(VNET_NODE_PKT_FLAG_NOT_ONLINE);
    stPkt.uiSrcNodeID = htonl(uiServerNid);

    return vnets_nodectrl_Send(uiDstSes, &stPkt, sizeof(stPkt));
}

