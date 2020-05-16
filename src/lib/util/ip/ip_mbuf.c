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

    /* 检查 IP 报文头的长度字段 */
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

    /* 如果偏移量为全1，即：0x1fff，则应为最后一个分片，即MF标志位不应被设置。如果MF被设置，为错误报文 */
    usOff = ntohs(pstIpHead->usOff);
    if (unlikely((0 != (usOff & IP_MF)) && (IP_OFFMASK == (usOff & IP_OFFMASK))))
    {
        return BS_ERR;
    }

    /* 如果该报文隐含的报文总长度大于65535字节，释放该IP报文 */
    usOff = (USHORT) ((usOff & IP_OFFMASK) << 3);
    if (unlikely(uiLen + usOff > IP_MAXPACKET))
    {
        return BS_ERR;
    }

    /* 目的地址合法性检查 */
    uiDstIp = ntohl(pstIpHead->unDstIp.uiIp);
    
    /* 按rfc，目的地址全0地址为非法，应丢弃。cisco的行为是放行。 */
    if (unlikely(0 == uiDstIp))
    {
        return BS_ERR;
    }

    /* 丢弃非法报文(除 255.255.255.255) */
    if (unlikely(IP_IN_BADCLASS(uiDstIp) && (INADDR_BROADCAST != uiDstIp)))
    {
        return BS_ERR;
    }

    if (unlikely(IP_IN_LOOPBACK(uiDstIp) || IP_IN_LOOPBACK(ntohl(pstIpHead->unSrcIp.uiIp))))
    {
        return BS_ERR;
    }

    /* 检查 MBUF 的数据长度是否等于 IP 报文的总长 */
    uiTotalLen = MBUF_TOTAL_DATA_LEN(pstMbuf);
    if (unlikely(uiTotalLen != uiLen))
    {
        if (unlikely(uiTotalLen < uiLen))
        {
            return BS_ERR;
        }
        else
        {
            /* 截去多余的长度 */
            MBUF_CutTail(pstMbuf, uiTotalLen - uiLen);
        }
    }

    return BS_OK;
}

/* 给Mbuf加上IP头 */
BS_STATUS IP_BuildIPHeader
(
    IN MBUF_S *pstMbuf,
    IN UINT uiDstIp/* 网络序 */,
    IN UINT uiSrcIp/* 网络序 */,
    IN UCHAR ucProto,
    IN USHORT usIdentification /* 网络序 */
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

