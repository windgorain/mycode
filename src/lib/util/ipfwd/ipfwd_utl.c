/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2013-1-10
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mbuf_utl.h"
#include "utl/ip_utl.h"
#include "utl/ip_mbuf.h"
#include "utl/eth_utl.h"
#include "utl/fib_utl.h"
#include "utl/ipfwd_utl.h"

typedef struct
{
    FIB_HANDLE hFib;
    USHORT usIdentification;
    UINT uiDbgFlag;
    PF_IPFWD_LINK_OUTPUT pfLinkOutput;
    PF_IPFWD_DeliverUp pfDeliverUp;
    
    IPFWD_SERVICE_HANDLE hIpFwdService;
}IPFWD_CTRL_S;


IPFWD_HANDLE IPFWD_Create
(
    IN FIB_HANDLE hFib,
    IN IPFWD_SERVICE_HANDLE hIpFwdService,
    IN PF_IPFWD_LINK_OUTPUT pfLinkOutput,
    IN PF_IPFWD_DeliverUp pfDeliverUp
)
{
    IPFWD_CTRL_S *pstCtrl;

    if (NULL == hFib)
    {
        return NULL;
    }

    pstCtrl = MEM_ZMalloc(sizeof(IPFWD_CTRL_S));
    if (NULL == pstCtrl)
    {
        return NULL;
    }

    pstCtrl->hFib = hFib;

    pstCtrl->hIpFwdService = hIpFwdService;
    pstCtrl->pfLinkOutput = pfLinkOutput;
    pstCtrl->pfDeliverUp = pfDeliverUp;

    return pstCtrl;
}

VOID IPFWD_Destory(IN IPFWD_HANDLE hIpFwd)
{
    IPFWD_CTRL_S *pstCtrl = hIpFwd;

    if (NULL == pstCtrl)
    {
        return;
    }

    MEM_Free(pstCtrl);
}

BS_STATUS IPFWD_BuildIpHeader
(
    IN IPFWD_HANDLE hIpFwd,
    IN MBUF_S *pstMbuf, /* 不带IP头 */
    IN UINT uiDstIp,    /* 网络序 */
    IN UINT uiSrcIp,    /* 网络序 */
    IN UCHAR ucProto
)
{
    IPFWD_CTRL_S *pstCtrl = hIpFwd;
    USHORT usIdentification;

    usIdentification = pstCtrl->usIdentification ++;
    usIdentification = htons(usIdentification);

    return IP_BuildIPHeader(pstMbuf, uiDstIp, uiSrcIp, ucProto, usIdentification);
}

/* 相比于OutPut, 不再填写IP头的东西,认为上面已经填写好了 */
BS_STATUS IPFWD_PreSend
(
    IN IPFWD_HANDLE hIpFwd,
    IN MBUF_S *pstMbuf /* 带IP头 */
)
{
    IP_HEAD_S *pstIpHead;
    UINT ulIfIndex;
    FIB_NODE_S stFibNode;
    IPFWD_CTRL_S *pstCtrl = hIpFwd;

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
    
    if (BS_OK != FIB_PrefixMatch(pstCtrl->hFib, pstIpHead->unDstIp.uiIp, &stFibNode))
    {
        return BS_NOT_FOUND;
    }

    MBUF_SET_SEND_IF_INDEX(pstMbuf, stFibNode.uiOutIfIndex);

    return BS_OK;
}


/* 相比于OutPut, 不再填写IP头的东西,认为上面已经填写好了 */
BS_STATUS IPFWD_Send
(
    IN IPFWD_HANDLE hIpFwd,
    IN MBUF_S *pstMbuf /* 带IP头 */
)
{
    IPFWD_CTRL_S *pstCtrl = hIpFwd;

    if (BS_OK != IPFWD_PreSend(hIpFwd, pstMbuf))
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    return pstCtrl->pfLinkOutput(MBUF_GET_SEND_IF_INDEX(pstMbuf), pstMbuf, htons(ETH_P_IP));
}

/* 填写IP头并发送数据 */
BS_STATUS IPFWD_Output
(
    IN IPFWD_HANDLE hIpFwd,
    IN MBUF_S *pstMbuf, /* 不带IP头 */
    IN UINT uiDstIp,    /* 网络序 */
    IN UINT uiSrcIp,    /* 网络序 */
    IN UCHAR ucProto
)
{
    if (BS_OK != IPFWD_BuildIpHeader(hIpFwd, pstMbuf, uiDstIp, uiSrcIp, ucProto))
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    return IPFWD_Send(hIpFwd, pstMbuf);
}

BS_STATUS IPFWD_Input (IN IPFWD_HANDLE hIpFwd, IN MBUF_S *pstMbuf)
{
    IP_HEAD_S *pstIpHead;
    IPFWD_SERVICE_RET_E eRet;
    FIB_NODE_S stFibNode;
    IPFWD_CTRL_S *pstCtrl = hIpFwd;

    if (BS_OK != IP_ValidPkt(pstMbuf))
    {
        BS_DBG_OUTPUT(pstCtrl->uiDbgFlag, IP_FWD_DBG_PACKET,
            ("IPFWD: Recv invalid ip packet.\r\n"));
        MBUF_Free (pstMbuf);
        return (BS_ERR);
    }

    pstIpHead = MBUF_MTOD (pstMbuf);

    BS_DBG_OUTPUT(pstCtrl->uiDbgFlag, IP_FWD_DBG_PACKET,
        ("IPFWD: Recv packet from %pI4 to %pI4\r\n",
        &pstIpHead->unSrcIp.uiIp, &pstIpHead->unDstIp.uiIp));

    if (NULL != pstCtrl->hIpFwdService)
    {
        eRet = IPFWD_Service_Handle(pstCtrl->hIpFwdService,
                    IPFWD_SERVICE_PRE_ROUTING, pstIpHead, pstMbuf);
        if (eRet == IPFWD_SERVICE_RET_TAKE_OVER)
        {
            return BS_OK;
        }
    }

    /* 判断该报文是否是全1、全0广播地址 */
    if ((pstIpHead->unDstIp.uiIp == 0xffffffff) || (pstIpHead->unDstIp.uiIp == 0))
    {
        BS_DBG_OUTPUT(pstCtrl->uiDbgFlag, IP_FWD_DBG_PACKET,
                ("IPFWD: DeliverUp broadcast packet.\r\n"));
        return pstCtrl->pfDeliverUp(pstIpHead, pstMbuf);
    }

    if (BS_OK != FIB_PrefixMatch(pstCtrl->hFib, pstIpHead->unDstIp.uiIp, &stFibNode))
    {
        if (NULL != pstCtrl->hIpFwdService)
        {
            eRet = IPFWD_Service_Handle(pstCtrl->hIpFwdService,
                        IPFWD_SERVICE_NO_FIB, pstIpHead, pstMbuf);
            if (eRet == IPFWD_SERVICE_RET_TAKE_OVER)
            {
                return BS_OK;
            }
        }

        BS_DBG_OUTPUT(pstCtrl->uiDbgFlag, IP_FWD_DBG_PACKET,
                ("IPFWD: Not find fib, discard packet.\r\n"));
        MBUF_Free(pstMbuf);
        return BS_OK;
    }

    BS_DBG_OUTPUT(pstCtrl->uiDbgFlag, IP_FWD_DBG_PACKET,
                ("IPFWD: Match fib %pI4/%d.\r\n",
                &stFibNode.stFibKey.uiDstOrStartIp, MASK_2_PREFIX(ntohl(stFibNode.stFibKey.uiMaskOrEndIp))));

    if (stFibNode.uiFlag & FIB_FLAG_DIRECT)
    {
        if (IP_IS_SUB_BROADCAST(pstIpHead->unDstIp.uiIp, stFibNode.stFibKey.uiMaskOrEndIp))
        {
            BS_DBG_OUTPUT(pstCtrl->uiDbgFlag, IP_FWD_DBG_PACKET,
                ("IPFWD: DeliverUp subnet broadcast packet.\r\n"));
            return pstCtrl->pfDeliverUp(pstIpHead, pstMbuf);
        }
    }

    if (stFibNode.uiFlag & FIB_FLAG_DELIVER_UP)
    {
        BS_DBG_OUTPUT(pstCtrl->uiDbgFlag, IP_FWD_DBG_PACKET,
                ("IPFWD: DeliverUp packet.\r\n"));
        return pstCtrl->pfDeliverUp(pstIpHead, pstMbuf);
    }

    if (NULL != pstCtrl->hIpFwdService)
    {
        eRet = IPFWD_Service_Handle(pstCtrl->hIpFwdService,
                    IPFWD_SERVICE_BEFORE_FORWARD, pstIpHead, pstMbuf);
        if (eRet == IPFWD_SERVICE_RET_TAKE_OVER)
        {
            return BS_OK;
        }
    }

    if (stFibNode.uiNextHop != 0)
    {
        MBUF_SET_NEXT_HOP(pstMbuf, stFibNode.uiNextHop);
    }
    else
    {
        MBUF_SET_NEXT_HOP(pstMbuf, pstIpHead->unDstIp.uiIp);
    }

    BS_DBG_OUTPUT(pstCtrl->uiDbgFlag, IP_FWD_DBG_PACKET,
        ("IPFWD: Forward packet to %08x.\r\n", stFibNode.uiOutIfIndex));

    return pstCtrl->pfLinkOutput(stFibNode.uiOutIfIndex, pstMbuf, htons(ETH_P_IP));
}


VOID IPFWD_SetDebug(IN IPFWD_HANDLE hIpFwd, IN IPFWD_HOW_DEBUG_E eHow, IN UINT uiDbgFlag)
{
    IPFWD_CTRL_S *pstCtrl = hIpFwd;

    switch (eHow)
    {
        case IPFWD_HOW_DEBUG_SET:
        {
            pstCtrl->uiDbgFlag = uiDbgFlag;
            break;
        }

        case IPFWD_HOW_DEBUG_ADD:
        {
            pstCtrl->uiDbgFlag |= uiDbgFlag;
            break;
        }

        case IPFWD_HOW_DEBUG_DEL:
        {
            pstCtrl->uiDbgFlag &= (~uiDbgFlag);
            break;
        }

        default:
        {
            BS_DBGASSERT(0);
            break;
        }
    }

    return;
}

