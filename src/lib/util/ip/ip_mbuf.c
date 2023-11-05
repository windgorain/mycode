/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"

#include "utl/ip_utl.h"
#include "utl/ip_mbuf.h"
#include "utl/eth_utl.h"
#include "utl/eth_mbuf.h"
#include "utl/in_checksum.h"

IP_HEAD_S * IP_GetIPHeaderByMbuf(IN MBUF_S *pstMbuf, IN NET_PKT_TYPE_E enPktType)
{
    UINT uiHeadLen = 0;
    NET_PKT_TYPE_E enPktTypeTmp = enPktType;
    ETH_PKT_INFO_S stPktInfo;
    UCHAR *pucData;
    IP_HEAD_S *pstIpHead = NULL;


    if (enPktTypeTmp == NET_PKT_TYPE_ETH)
    {
        if (BS_OK != ETH_GetEthHeadInfoByMbuf(pstMbuf, &stPktInfo))
        {
            return NULL;
        }

        if (stPktInfo.usType != ETH_P_IP)
        {
            return NULL;
        }

        enPktTypeTmp = NET_PKT_TYPE_IP;
        uiHeadLen += stPktInfo.usHeadLen;
    }

    if (enPktTypeTmp == NET_PKT_TYPE_IP)
    {
        if (BS_OK != MBUF_MakeContinue(pstMbuf, uiHeadLen + sizeof(IP_HEAD_S)))
        {
            return NULL;
        }

        pucData = MBUF_MTOD(pstMbuf);
        pstIpHead = (IP_HEAD_S*)(pucData + uiHeadLen);

        if (BS_OK != MBUF_MakeContinue(pstMbuf, uiHeadLen + IP_HEAD_LEN(pstIpHead)))
        {
            return NULL;
        }
    }

    return pstIpHead;
}

BS_STATUS IP_ValidPkt(IN MBUF_S *pstMbuf)
{
    IP_HEAD_S *pstIpHead;
    UINT uiHLen;
    UINT uiLen;
    UINT uiDstIp;
    UINT uiTotalLen;
    USHORT usOff;

    if (BS_OK != MBUF_MakeContinue (pstMbuf, sizeof(IP_HEAD_S)))
    {
        MBUF_Free (pstMbuf);
        return (BS_ERR);
    }

    pstIpHead = MBUF_MTOD (pstMbuf);

    uiHLen = IP_HEAD_LEN(pstIpHead);

    
    if (unlikely(sizeof(IP_HEAD_S) != uiHLen))
    {
        if (unlikely(uiHLen < sizeof(IP_HEAD_S)))
        {
            return BS_ERR;
        }
        else
        {
            if (unlikely(BS_OK != MBUF_MakeContinue(pstMbuf, uiHLen)))
            {
                return BS_ERR;
            }
            
            pstIpHead = MBUF_MTOD (pstMbuf);
        }
    }

    if (unlikely(IP_VERSION != pstIpHead->ucVer))
    {
        return BS_ERR;
    }

    uiLen = ntohs(pstIpHead->usTotlelen);
    if (unlikely(uiLen < uiHLen))
    {
        return BS_ERR;
    }

    
    usOff = ntohs(pstIpHead->usOff);
    if (unlikely((0 != (usOff & IP_MF)) && (IP_OFFMASK == (usOff & IP_OFFMASK))))
    {
        return BS_ERR;
    }

    
    usOff = (USHORT) ((usOff & IP_OFFMASK) << 3);
    if (unlikely(uiLen + usOff > IP_MAXPACKET))
    {
        return BS_ERR;
    }

    
    uiDstIp = ntohl(pstIpHead->unDstIp.uiIp);
    
    
    if (unlikely(0 == uiDstIp))
    {
        return BS_ERR;
    }

    
    if (unlikely(IP_IN_BADCLASS(uiDstIp) && (INADDR_BROADCAST != uiDstIp)))
    {
        return BS_ERR;
    }

    if (unlikely(IP_IN_LOOPBACK(uiDstIp) || IP_IN_LOOPBACK(ntohl(pstIpHead->unSrcIp.uiIp))))
    {
        return BS_ERR;
    }

    
    uiTotalLen = MBUF_TOTAL_DATA_LEN(pstMbuf);
    if (unlikely(uiTotalLen != uiLen))
    {
        if (unlikely(uiTotalLen < uiLen))
        {
            return BS_ERR;
        }
        else
        {
            
            MBUF_CutTail(pstMbuf, uiTotalLen - uiLen);
        }
    }

    return BS_OK;
}


BS_STATUS IP_BuildIPHeader
(
    IN MBUF_S *pstMbuf,
    IN UINT uiDstIp,
    IN UINT uiSrcIp,
    IN UCHAR ucProto,
    IN USHORT usIdentification 
)
{
    IP_HEAD_S *pstIpHead;
    USHORT usTotleLen;

    if (BS_OK != MBUF_Prepend(pstMbuf, sizeof(IP_HEAD_S)))
    {
        return BS_NO_MEMORY;
    }

    if (BS_OK != MBUF_MakeContinue(pstMbuf, sizeof(IP_HEAD_S)))
    {
        return BS_NO_MEMORY;
    }

    pstIpHead = MBUF_MTOD(pstMbuf);

    Mem_Zero(pstIpHead, sizeof(IP_HEAD_S));

    usTotleLen = MBUF_TOTAL_DATA_LEN(pstMbuf);

    pstIpHead->ucVer = IP_VERSION;
    pstIpHead->usTotlelen = htons(usTotleLen);
    pstIpHead->usIdentification = usIdentification;
    pstIpHead->ucHLen = (sizeof (IP_HEAD_S) >> 2);
    pstIpHead->ucProto = ucProto;
    pstIpHead->unSrcIp.uiIp = uiSrcIp;
    pstIpHead->unDstIp.uiIp = uiDstIp;
    pstIpHead->ucTtl = 128;
    pstIpHead->usCrc = 0;
    pstIpHead->usCrc = IP_CheckSum((UCHAR*)pstIpHead, IP_HEAD_LEN(pstIpHead));

    return BS_OK;
}

