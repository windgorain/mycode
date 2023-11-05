/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/net.h"
#include "utl/eth_utl.h"
#include "utl/ip_utl.h"
#include "utl/in_checksum.h"
#include "utl/ip_string.h"


USHORT IP_CheckSum (IN UCHAR *pucBuf, IN UINT ulLen)
{
    return IN_CHKSUM_CheckSum(pucBuf, ulLen);
}

IP_HEAD_S * IP_GetIPHeader(IN UCHAR *pucData, IN UINT uiDataLen, IN NET_PKT_TYPE_E enPktType)
{
    NET_PKT_TYPE_E enPktTypeTmp = enPktType;
    ETH_PKT_INFO_S stPktInfo;
    UINT uiHeadLen = 0;
    IP_HEAD_S *pstIpHead = NULL;

    if (enPktTypeTmp == NET_PKT_TYPE_ETH)
    {
        if (BS_OK != ETH_GetEthHeadInfo(pucData, uiDataLen, &stPktInfo))
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
        if (uiHeadLen + sizeof(IP_HEAD_S) > uiDataLen)
        {
            return NULL;
        }

        pstIpHead = (IP_HEAD_S*)(pucData + uiHeadLen);

        if (uiHeadLen + IP_HEAD_LEN(pstIpHead) > uiDataLen)
        {
            return NULL;
        }
    }

    return pstIpHead;
}

BOOL_T IPUtl_IsExistInIpArry(IN IP_MASK_S *pstIpMask, IN UINT uiNum, IN UINT uiIP, IN UINT uiMask)
{
    UINT i;

    for(i=0; i<uiNum; i++)
    {
        if ((pstIpMask[i].uiIP == uiIP) && (pstIpMask[i].uiMask == uiMask))
        {
            return TRUE;
        }
    }

    return FALSE;
}

char * inet_ntop4(const struct in_addr *addr, char *buf, socklen_t len)
{
    const u_int8_t *addr_p = (const u_int8_t *)&addr->s_addr;
    char tmp[INET_ADDRSTRLEN]; 
    int fulllen;

    fulllen = scnprintf(tmp, sizeof(tmp), "%d.%d.%d.%d",
            addr_p[0], addr_p[1], addr_p[2], addr_p[3]);
    if (fulllen >= (int)len) {
        return NULL;
    }

    memcpy(buf, tmp, fulllen + 1);

    return buf;
}


BOOL_T IP_IsPrivateIp(UINT ip)
{
    UCHAR *pucData = (void*)&ip;

    if(10 == pucData[0]) {
        return TRUE;
    }

    if (172 == pucData[0])
    {
        if (pucData[1] >= 16 && pucData[1] <= 31)
        {
            return TRUE;
        }
    }

    if((192 == pucData[0]) && (168 == pucData[1])) {
        return TRUE;
    }

    if((169 == pucData[0]) && (254 == pucData[1])) {
        return TRUE;
    }

    return FALSE;
}
