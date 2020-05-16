/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2017-5-20
* Description: IP String库
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/net.h"
#include "utl/eth_utl.h"
#include "utl/lstr_utl.h"
#include "utl/txt_utl.h"
#include "utl/data2hex_utl.h"
#include "utl/ip_utl.h"
#include "utl/socket_utl.h"
#include "utl/ip_string.h"

/* 解析字符串 IP/Mask */
BS_STATUS IPString_ParseIpMask(CHAR *pcIpMaskString, OUT IP_MAKS_S *pstIpMask)
{
    LSTR_S stIP;
    LSTR_S stMask;
    CHAR szTmp[32];
    
    TXT_StrSplit(pcIpMaskString, '/', &stIP, &stMask);

    LSTR_Strim(&stIP, TXT_BLANK_CHARS, &stIP);
    LSTR_Strim(&stMask, TXT_BLANK_CHARS, &stMask);

    if ((stIP.uiLen > 15) || (stMask.uiLen > 15))
    {
        return BS_BAD_PARA;
    }

    LSTR_Strlcpy(&stIP, sizeof(szTmp), szTmp);
    pstIpMask->uiIP = Socket_Ipsz2IpHost(szTmp);

    LSTR_Strlcpy(&stMask, sizeof(szTmp), szTmp);
    pstIpMask->uiMask = Socket_Ipsz2IpHost(szTmp);

    return BS_OK;
}

/* 解析字符串IP/Mask列表,比如:1.1.1.1/255.0.0.0,2.1.1.1./255.0.0.0,
   返回值: IP个数
*/
UINT IPString_ParseIpMaskList(IN CHAR *pcIpMaskString, IN CHAR cSplitChar, IN UINT uiIpMaskMaxNum, OUT IP_MAKS_S *pstIpMasks)
{
    CHAR *pcIpMask;
    UINT uiNum = 0;

    TXT_SCAN_ELEMENT_BEGIN(pcIpMaskString, cSplitChar, pcIpMask)
    {
        if (uiNum >= uiIpMaskMaxNum)
        {
            TXT_SCAN_ELEMENT_STOP();
            return uiNum;
        }

        IPString_ParseIpMask(pcIpMask, &pstIpMasks[uiNum]);

        uiNum ++;
    }TXT_SCAN_ELEMENT_END();

    return uiNum;
}

CHAR * IPString_IP2String(IN UINT ip/*net order*/, OUT CHAR *str)
{
    UCHAR *pucData = (void*)&ip;

    int len = sprintf(str, "%u.%u.%u.%u", pucData[0], pucData[1], pucData[2], pucData[3]);
    pucData[len] = 0;

    return str;
}

CHAR * IPString_IPHeader2String(IN VOID *ippkt, OUT CHAR *info, IN UINT infosize)
{
    IP_HEAD_S *iph;
    char sip[32];
    char dip[32];

    iph = ippkt;

    snprintf(info, infosize,
            "\"ip_head_len\":%u,\"ip_tos\":%u,\"ip_total_length\":%u,\"ip_pkt_id\":%d,"
            "\"ip_flag\":%u,\"ip_frag_offset\":%u,\"ip_ttl\":%u,\"ip_protocol\":%u,\"ip_head_chksum\":\"%04x\","
            "\"sip\":\"%s\",\"dip\":\"%s\"",
            iph->ucHLen*4, iph->ucTos, ntohs(iph->usTotlelen), ntohs(iph->usIdentification),
            IP_HEAD_FLAG(iph), IP_HEAD_FRAG_OFFSET(iph), iph->ucTtl, iph->ucProto, iph->usCrc,
            IPString_IP2String(iph->unSrcIp.uiIp, sip), IPString_IP2String(iph->unDstIp.uiIp, dip));

    return info;
}

CHAR * IPString_IPHeader2Hex(IN VOID *ippkt, OUT CHAR *info)
{
    IP_HEAD_S *iph = ippkt;
    int len = iph->ucHLen*4;

    DH_Data2HexString(ippkt, len, info);

    return info;
}


