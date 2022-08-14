/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-1-22
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/ip_utl.h"
#include "utl/exec_utl.h"
#include "utl/ip_mbuf.h"
#include "utl/sif_utl.h"
#include "utl/fib_utl.h"
#include "utl/eth_utl.h"
#include "utl/ipfwd_service.h"
#include "comp/comp_wan.h"
#include "comp/comp_if.h"
#include "comp/comp_acl.h"

#include "../h/wan_ifnet.h"
#include "../h/wan_ipfwd_service.h"
#include "../h/wan_fib.h"
#include "../h/wan_deliver_up.h"
#include "../h/wan_ipfwd.h"

/* Debug 选项 */
#define WAN_IP_FWD_DBG_PACKET 0x1

#define _WAN_IPFWD_DBG_WITH_ACL(_flag, _switch, _pstIpHead, _X) \
    do { \
        if ((_flag) & (_switch)) {  \
            if (_wan_ipfwd_IsPermitDbgOutput(_pstIpHead)) { \
                BS_DBG_OUTPUT(_flag, _switch, _X);  \
            }   \
        }   \
    }while(0)

static UINT g_uiWanIpFwdDbgFlag = 0;
static UINT64 g_ulWanIpFwdDbgAcl = ACL_INVALID_LIST_ID;

static BOOL_T _wan_ipfwd_IsPermitDbgOutput(IN IP_HEAD_S *pstIpHead)
{
    IPACL_MATCH_INFO_S stMatchInfo;

    if (g_ulWanIpFwdDbgAcl == ACL_INVALID_LIST_ID)
    {
        return TRUE;
    }

    if (NULL == pstIpHead)
    {
        return FALSE;
    }

    stMatchInfo.uiDIP = ntohl(pstIpHead->unDstIp.uiIp);
    stMatchInfo.uiSIP = ntohl(pstIpHead->unSrcIp.uiIp);
    stMatchInfo.uiKeyMask = IPACL_KEY_SIP|IPACL_KEY_DIP;
    
    if (BS_ACTION_PERMIT == ACL_Match(0, ACL_TYPE_IP, g_ulWanIpFwdDbgAcl, &stMatchInfo))
    {
        return TRUE;
    }

    return FALSE;
}

BS_STATUS WAN_IpFwd_Input (IN MBUF_S *pstMbuf)
{
    UINT uiVrfID;
    IP_HEAD_S *pstIpHead;
    IPFWD_SERVICE_RET_E eServiceRet;
    FIB_NODE_S stFibNode;
    CHAR szIfName[IF_MAX_NAME_LEN + 1];

    uiVrfID = MBUF_GET_INVPNID(pstMbuf);

    if (BS_OK != IP_ValidPkt(pstMbuf))
    {
        _WAN_IPFWD_DBG_WITH_ACL(g_uiWanIpFwdDbgFlag, WAN_IP_FWD_DBG_PACKET, NULL, 
            ("IPFWD: Recv invalid ip packet.\r\n"));
        MBUF_Free (pstMbuf);
        return (BS_ERR);
    }

    pstIpHead = MBUF_MTOD (pstMbuf);

    _WAN_IPFWD_DBG_WITH_ACL(g_uiWanIpFwdDbgFlag, WAN_IP_FWD_DBG_PACKET, pstIpHead, 
        ("IPFWD: Recv packet from %pI4 to %pI4\r\n",
        &pstIpHead->unSrcIp.uiIp, &pstIpHead->unDstIp.uiIp));

    eServiceRet = WAN_IpFwdService_Process(IPFWD_SERVICE_PRE_ROUTING, pstIpHead, pstMbuf);
    if (eServiceRet == IPFWD_SERVICE_RET_TAKE_OVER)
    {
        return BS_OK;
    }

    /* 判断该报文是否是全1、全0广播地址 */
    if ((pstIpHead->unDstIp.uiIp == 0xffffffff) || (pstIpHead->unDstIp.uiIp == 0))
    {
        _WAN_IPFWD_DBG_WITH_ACL(g_uiWanIpFwdDbgFlag, WAN_IP_FWD_DBG_PACKET, pstIpHead,
                ("IPFWD: DeliverUp broadcast packet.\r\n"));
        return WAN_DeliverUp(pstIpHead, pstMbuf);
    }

    if (BS_OK != WanFib_PrefixMatch(uiVrfID, pstIpHead->unDstIp.uiIp, &stFibNode))
    {
        eServiceRet = WAN_IpFwdService_Process(IPFWD_SERVICE_NO_FIB, pstIpHead, pstMbuf);
        if (eServiceRet == IPFWD_SERVICE_RET_TAKE_OVER)
        {
            return BS_OK;
        }

        _WAN_IPFWD_DBG_WITH_ACL(g_uiWanIpFwdDbgFlag, WAN_IP_FWD_DBG_PACKET, pstIpHead,
                ("IPFWD: Not find fib, discard packet.\r\n"));
        MBUF_Free(pstMbuf);
        return BS_OK;
    }

    _WAN_IPFWD_DBG_WITH_ACL(g_uiWanIpFwdDbgFlag, WAN_IP_FWD_DBG_PACKET, pstIpHead,
                ("IPFWD: Match fib %pI4/%d.\r\n",
                &stFibNode.stFibKey.uiDstOrStartIp, MASK_2_PREFIX(ntohl(stFibNode.stFibKey.uiMaskOrEndIp))));

    if (stFibNode.uiFlag & FIB_FLAG_DELIVER_UP)
    {
        _WAN_IPFWD_DBG_WITH_ACL(g_uiWanIpFwdDbgFlag, WAN_IP_FWD_DBG_PACKET, pstIpHead,
                ("IPFWD: DeliverUp packet.\r\n"));
        return WAN_DeliverUp(pstIpHead, pstMbuf);
    }

    if (stFibNode.uiFlag & FIB_FLAG_DIRECT)
    {
        if (IP_IS_SUB_BROADCAST(pstIpHead->unDstIp.uiIp, stFibNode.stFibKey.uiMaskOrEndIp))
        {
            _WAN_IPFWD_DBG_WITH_ACL(g_uiWanIpFwdDbgFlag, WAN_IP_FWD_DBG_PACKET, pstIpHead,
                ("IPFWD: DeliverUp subnet broadcast packet.\r\n"));
            return WAN_DeliverUp(pstIpHead, pstMbuf);
        }
    }

    if (stFibNode.uiNextHop == 0)
    {
        MBUF_SET_NEXT_HOP(pstMbuf, pstIpHead->unDstIp.uiIp);
    }
    else
    {
        MBUF_SET_NEXT_HOP(pstMbuf, stFibNode.uiNextHop);
    }

    MBUF_SET_SEND_IF_INDEX(pstMbuf, stFibNode.uiOutIfIndex);

    eServiceRet = WAN_IpFwdService_Process(IPFWD_SERVICE_BEFORE_FORWARD, pstIpHead, pstMbuf);
    if (eServiceRet == IPFWD_SERVICE_RET_TAKE_OVER)
    {
        return BS_OK;
    }

    _WAN_IPFWD_DBG_WITH_ACL(g_uiWanIpFwdDbgFlag, WAN_IP_FWD_DBG_PACKET, pstIpHead,
        ("IPFWD: Forward packet to %s.\r\n", IFNET_GetIfName(stFibNode.uiOutIfIndex, szIfName)));

    return IFNET_LinkOutput(stFibNode.uiOutIfIndex, pstMbuf, htons(ETH_P_IP));
}

/* 相比于OutPut, 不再填写IP头的东西,认为上面已经填写好了 */
static BS_STATUS wan_ipfwd_PreSend
(
    IN MBUF_S *pstMbuf /* 带IP头 */
)
{
    IP_HEAD_S *pstIpHead;
    UINT ulIfIndex;
    FIB_NODE_S stFibNode;
    UINT uiVrfID;

    /* 是否指定了出接口 */
    ulIfIndex = MBUF_GET_SEND_IF_INDEX(pstMbuf);
    if (0 != ulIfIndex)
    {
        return BS_OK;
    }

    if (BS_OK != MBUF_MakeContinue (pstMbuf, sizeof(IP_HEAD_S)))
    {
        return (BS_ERR);
    }

    pstIpHead = MBUF_MTOD (pstMbuf);

    uiVrfID = MBUF_GET_OUTVPNID(pstMbuf);
    
    if (BS_OK != WanFib_PrefixMatch(uiVrfID, pstIpHead->unDstIp.uiIp, &stFibNode))
    {
        _WAN_IPFWD_DBG_WITH_ACL(g_uiWanIpFwdDbgFlag, WAN_IP_FWD_DBG_PACKET, pstIpHead,
            ("IPFWD: Can't match fib for packet, sip=%pI4, dip=%pI4.\r\n",
            &pstIpHead->unSrcIp.uiIp, &pstIpHead->unDstIp.uiIp));
        return BS_NOT_FOUND;
    }

    MBUF_SET_SEND_IF_INDEX(pstMbuf, stFibNode.uiOutIfIndex);

    if (stFibNode.uiNextHop != 0)
    {
        MBUF_SET_NEXT_HOP(pstMbuf, stFibNode.uiNextHop);
    }
    else
    {
        MBUF_SET_NEXT_HOP(pstMbuf, pstIpHead->unDstIp.uiIp);
    }

    return BS_OK;
}


/* 相比于OutPut, 不再填写IP头的东西,认为上面已经填写好了 */
BS_STATUS WAN_IpFwd_Send(IN MBUF_S *pstMbuf)
{
    UINT uiIfIndex;
    CHAR szIfName[IF_MAX_NAME_LEN + 1];
    IP_HEAD_S *pstIpHead;

    if (BS_OK != wan_ipfwd_PreSend(pstMbuf))
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    pstIpHead = MBUF_MTOD(pstMbuf);

    uiIfIndex = MBUF_GET_SEND_IF_INDEX(pstMbuf);

    _WAN_IPFWD_DBG_WITH_ACL(g_uiWanIpFwdDbgFlag, WAN_IP_FWD_DBG_PACKET, pstIpHead,
        ("IPFWD: Forward packet to %s, sip=%pI4, dip=%pI4.\r\n",
        IFNET_GetIfName(uiIfIndex, szIfName), &pstIpHead->unSrcIp.uiIp, &pstIpHead->unDstIp.uiIp));

    return IFNET_LinkOutput(uiIfIndex, pstMbuf, htons(ETH_P_IP));
}

BS_STATUS WAN_IpFwd_Output
(
    IN MBUF_S *pstMbuf,
    IN UINT uiDstIp,    /* 网络序 */
    IN UINT uiSrcIp,    /* 网络序 */
    IN UCHAR ucProto
)
{
    static USHORT usIdentification = 0;
    USHORT usId;

    usId = usIdentification ++;
    usId = htons(usId);

    if (BS_OK != IP_BuildIPHeader(pstMbuf, uiDstIp, uiSrcIp, ucProto, usId))
    {
        _WAN_IPFWD_DBG_WITH_ACL(g_uiWanIpFwdDbgFlag, WAN_IP_FWD_DBG_PACKET, NULL,
            ("IPFWD: Build IP header failed, SrcIP:%pI4, DstIP:%pI4.\r\n", &uiSrcIp, &uiDstIp));
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    return WAN_IpFwd_Send(pstMbuf);
}

/*
debug ip packet [acl xxx]
*/
PLUG_API BS_STATUS WAN_IpFwd_DebugPacket
(
    IN UINT ulArgc,
    IN CHAR **argv
)
{
    ACL_NAME_ID_S stAclNameID;
    
    if (ulArgc >= 5)
    {
        if (BS_OK != ACL_Ioctl(0, ACL_TYPE_IP, COMP_ACL_IOCTL_ADD_LIST_REF, argv[4]))
        {
            EXEC_OutString("Acl not exist.\r\n");
            return BS_ERR;
        }

        stAclNameID.pcAclListName = argv[4];
        ACL_Ioctl(0, ACL_TYPE_IP, COMP_ACL_IOCTL_GET_LIST_ID, &stAclNameID);

        if (g_ulWanIpFwdDbgAcl != ACL_INVALID_LIST_ID)
        {
            ACL_Ioctl(0, ACL_TYPE_IP, COMP_ACL_IOCTL_DEL_LIST_REF_BY_ID, &g_ulWanIpFwdDbgAcl);
            g_ulWanIpFwdDbgAcl = ACL_INVALID_LIST_ID;
        }

        g_ulWanIpFwdDbgAcl = stAclNameID.ulAclListID;
    }

    g_uiWanIpFwdDbgFlag |= WAN_IP_FWD_DBG_PACKET;

    return BS_OK;
}

/*
no debug ip packet
*/
PLUG_API BS_STATUS WAN_IpFwd_NoDebugPacket
(
    IN UINT ulArgc,
    IN CHAR **argv
)
{
    g_uiWanIpFwdDbgFlag &= ~WAN_IP_FWD_DBG_PACKET;

    if (g_ulWanIpFwdDbgAcl != ACL_INVALID_LIST_ID)
    {
        ACL_Ioctl(0, ACL_TYPE_IP, COMP_ACL_IOCTL_DEL_LIST_REF_BY_ID, &g_ulWanIpFwdDbgAcl);
        g_ulWanIpFwdDbgAcl = ACL_INVALID_LIST_ID;
    }

    return BS_OK;
}


