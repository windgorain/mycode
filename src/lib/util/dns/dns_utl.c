/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2013-4-8
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/dns_utl.h"


static USHORT g_usDnsTID; /* DNS标识 */

/* 返回解析出了多少个Label, 如果返回0表示出错 */
UINT DNS_DomainName2Labels(IN CHAR *pcDomainName, OUT DNS_LABELS_S *pstLabels)
{
    UINT uiLabelNum;
    UINT i;
    UINT uiLen;
    
    if ((pcDomainName == NULL) || (pcDomainName[0] == '\0'))
    {
        return 0;
    }

    if (strlen(pcDomainName) > DNS_MAX_DOMAIN_NAME_LEN)
    {
        return 0;
    }

    uiLabelNum = TXT_StrToToken(pcDomainName, ".", pstLabels->apcLabels, DNS_MAX_LABEL_NUM);

    for (i=0; i<uiLabelNum; i++)
    {
        uiLen = strlen(pstLabels->apcLabels[i]);
        if ((uiLen == 0) || (uiLen > DNS_MAX_LABEL_LEN))
        {
            return 0;
        }
    }

    return uiLabelNum;
}

/* 获得DNS的数字分格式的域名的长度,包含开头的第一个数字和最后的'\0' . 返回0表示出错 */
UINT DNS_GetDnsNameSize(IN UCHAR *pucData, IN UINT uiMaxSize)
{
    UINT uiDnsNameSize = 0;
    UINT uiLableLen;
    UCHAR *pucDataTmp = pucData;
    UINT uiDnsNameMaxSize;

    uiDnsNameMaxSize = MIN(uiMaxSize, DNS_MAX_DNS_NAME_SIZE);

    if (uiDnsNameMaxSize < 2)
    {
        return 0;
    }

    if ((pucDataTmp[0] & 0xc0) == 0xc0)
    {
        return 2;
    }

    while (*pucDataTmp != 0)
    {
        uiLableLen = *pucDataTmp;
        uiDnsNameSize += (uiLableLen + 1);

        if (uiDnsNameSize >= uiDnsNameMaxSize)
        {
            return 0;
        }

        pucDataTmp = pucData + uiDnsNameSize;
    }

    return uiDnsNameSize + 1/* 1为最后的一个'\0' */;
}

/* 将普通的点分格式的域名转成DNS的数字分格式的域名. 返回组装后的长度(含'\0'). 返回0为失败 */
UINT DNS_DomainName2DnsName
(
    IN CHAR *pcDomainName,
    OUT CHAR *pcDnsDomainName,
    IN UINT uiDnsDomainNameSize
)
{
    CHAR *pcTmp = pcDomainName;
    UINT uiLableCharCount = 0;
    UINT uiCountOffset = 0;
    CHAR cTmp;
    UINT uiDomainNameLen;

    pcDnsDomainName[0] = '\0';

    uiDomainNameLen = strlen(pcDomainName);

    if ((uiDomainNameLen > DNS_MAX_DOMAIN_NAME_LEN) || (uiDomainNameLen + 1 >= uiDnsDomainNameSize))
    {
        return 0;
    }

    while (1)
    {
        cTmp = *pcTmp;
        if ((cTmp != '.') && (cTmp != '\0'))
        {
            uiLableCharCount ++;
            pcDnsDomainName[uiCountOffset + uiLableCharCount] = cTmp;
        }
        else
        {
            if (uiLableCharCount > DNS_MAX_LABEL_LEN)
            {
                pcDnsDomainName[0] = '\0';
                return 0;
            }

            pcDnsDomainName[uiCountOffset] = uiLableCharCount;
            if (uiLableCharCount > 0)
            {
                uiCountOffset += (uiLableCharCount + 1);
                uiLableCharCount = 0;
            }
        }

        if (cTmp == '\0')
        {
            break;
        }

        pcTmp ++;
    }

    pcDnsDomainName[uiCountOffset] = '\0';

    return uiCountOffset + 1;
}

/* 返回第一个RR的指针, 包含这个RR的域名 */
UCHAR * DNS_GetFirstRR(IN DNS_HEADER_S *pstDnsHeader, IN UINT uiDataLen)
{
    UINT uiPrefixLen;
    UCHAR *pucDnsName;
    UINT uiDnsNameSize;
    UCHAR *pucRRData;

    if (uiDataLen <= sizeof(DNS_HEADER_S))
    {
        return NULL;
    }

    if (pstDnsHeader->usQdCount == 0)
    {
        return NULL;
    }

    if (pstDnsHeader->usAnCount == 0)
    {
        return NULL;
    }

    uiPrefixLen = sizeof(DNS_HEADER_S);

    pucDnsName = (UCHAR*)(pstDnsHeader + 1);
    uiDnsNameSize = DNS_GetDnsNameSize(pucDnsName, uiDataLen - sizeof(DNS_HEADER_S));
    if (uiDnsNameSize == 0)
    {
        return NULL;
    }

    uiPrefixLen += uiDnsNameSize;
    uiPrefixLen += sizeof(DNS_QST_S);

    if (uiDataLen <= uiPrefixLen)
    {
        return NULL;
    }

    pucRRData = ((UCHAR*)pstDnsHeader) + uiPrefixLen;

    return pucRRData;
}

/* 根据包含域名的RR获得其RD的长度和指针, 返回值为RD指针 */
UCHAR * DNS_GetRDByRR(IN UCHAR *pucRRWithDomain, IN UINT uiDataLen, OUT USHORT *pusRdLen)
{
    UINT uiDnsNameSize;
    DNS_RR_S *pstRR;
    USHORT usRdLen;
    UINT uiPrefixLen;

    uiDnsNameSize = DNS_GetDnsNameSize(pucRRWithDomain, uiDataLen);
    if (uiDnsNameSize == 0)
    {
        return NULL;
    }

    uiPrefixLen = uiDnsNameSize;

    if(uiDataLen < uiPrefixLen + sizeof(DNS_RR_S))
    {
        return NULL;
    }

    pstRR = (DNS_RR_S*)(pucRRWithDomain + uiPrefixLen);
    usRdLen = ntohs(pstRR->usRDLen);
    uiPrefixLen += sizeof(DNS_RR_S);

    if (uiDataLen < uiPrefixLen + usRdLen)
    {
        return NULL;
    }

    *pusRdLen = usRdLen;

    return pucRRWithDomain + uiPrefixLen;
}


/* 返回构造之后的报文长度 */
UINT DNS_BuildQuestPacket
(
    IN CHAR *pcDomainName,
    IN USHORT usType,
    OUT DNS_PKT_S *pstPkt
)
{
    UINT uiLen;
    CHAR *pucData;
    DNS_HEADER_S *pstDnsHeader;
    UINT uiRemainSize;
    DNS_QST_S *pstQst;

    if (strlen(pcDomainName) > DNS_MAX_DOMAIN_NAME_LEN)
    {
        return 0;
    }

    pstDnsHeader = (DNS_HEADER_S*)(pstPkt->aucData);
    uiRemainSize = sizeof(DNS_PKT_S);

    Mem_Zero(pstDnsHeader, sizeof(DNS_HEADER_S));

    pstDnsHeader->usTransID = g_usDnsTID ++;
    pstDnsHeader->usRD = 1;
    pstDnsHeader->usQdCount = htons(1);

    pucData = (CHAR*)(pstDnsHeader + 1);
    uiRemainSize -= sizeof(DNS_HEADER_S);
    uiLen = DNS_DomainName2DnsName(pcDomainName, pucData, uiRemainSize);

    pucData += uiLen;
    uiRemainSize -= uiLen;

    pstQst = (DNS_QST_S *)pucData;

    pstQst->usType = htons(usType);
    pstQst->usClass = htons(DNS_CLASS_IN);

    return sizeof(DNS_HEADER_S) + uiLen + 4;
}

/* 返回构造之后的数据长度 */
UINT DNS_BuildRR
(
    IN CHAR *pcDomainName,
    IN USHORT usType,
    IN USHORT usClass,
    IN UINT uiTTL,  /* 秒 */
    IN USHORT usRDLen,
    IN UCHAR *pucRDData,
    OUT UCHAR *pucData,
    IN UINT uiDataSize
)
{
    UINT uiLen;
    DNS_RR_S *pstRR;
    UCHAR *pucDataTmp;
    UINT uiDomainNameLen;
    UINT uiBuildSize; /* 构造之后的长度 */

    uiDomainNameLen = strlen(pcDomainName);

    if (uiDomainNameLen > DNS_MAX_DOMAIN_NAME_LEN)
    {
        return 0;
    }

    uiBuildSize = uiDomainNameLen + 2 + sizeof(DNS_RR_S) + usRDLen;

    if (uiBuildSize > uiDataSize)
    {
        return 0;
    }

    pucDataTmp = pucData;
    
    uiLen = DNS_DomainName2DnsName(pcDomainName, (CHAR*)pucDataTmp, uiDataSize);
    if (uiLen == 0)
    {
        return 0;
    }

    pucDataTmp += uiLen;

    pstRR = (DNS_RR_S*)pucDataTmp;
    memset(pstRR, 0, sizeof(DNS_RR_S));
    pstRR->usType = htons(usType);
    pstRR->usClass = htons(usClass);
    pstRR->uiTTL = htonl(uiTTL);
    pstRR->usRDLen = htons(usRDLen);

    pucDataTmp += sizeof(DNS_RR_S);
    memcpy(pucDataTmp, pucRDData, usRDLen);

    return uiBuildSize;
}

/* 返回构造之后的数据长度. 返回0表示失败 */
UINT DNS_BuildRRCompress
(
    IN USHORT usDomainNameOffset,
    IN USHORT usType,
    IN USHORT usClass,
    IN UINT uiTTL,  /* 秒 */
    IN USHORT usRDLen,
    IN UCHAR *pucRDData,
    OUT UCHAR *pucData,
    IN UINT uiDataSize
)
{
    DNS_RR_S *pstRR;
    UCHAR *pucDataTmp;
    UINT uiBuildSize; /* 构造之后的长度 */
    USHORT *pusDomainNameCompress;

    uiBuildSize = 2 + sizeof(DNS_RR_S) + usRDLen;

    if (uiBuildSize > uiDataSize)
    {
        return 0;
    }

    pucDataTmp = pucData;
    pusDomainNameCompress = (USHORT*)pucDataTmp;
    usDomainNameOffset |= 0xC000;
    *pusDomainNameCompress  = htons(usDomainNameOffset);
    pucDataTmp += 2;

    pstRR = (DNS_RR_S*)pucDataTmp;
    memset(pstRR, 0, sizeof(DNS_RR_S));
    pstRR->usType = htons(usType);
    pstRR->usClass = htons(usClass);
    pstRR->uiTTL = htonl(uiTTL);
    pstRR->usRDLen = htons(usRDLen);

    pucDataTmp += sizeof(DNS_RR_S);
    memcpy(pucDataTmp, pucRDData, usRDLen);

    return uiBuildSize;
}

int DNS_GetDomainNameByPacket
(
    IN UCHAR *pucDnsPkt,
    IN UINT uiPktLen,
    OUT CHAR szDomainName[DNS_MAX_DOMAIN_NAME_LEN + 1]
)
{
    UCHAR *pucData;
    UINT uiLen;
    CHAR *pcDst;
    UCHAR *pucTail;
    UINT uiDomainNameLen = 0;

    szDomainName[0] = '\0';

    if (uiPktLen <= sizeof(DNS_HEADER_S))
    {
        return BS_ERR;
    }

    pucData = pucDnsPkt + sizeof(DNS_HEADER_S);
    pcDst = szDomainName;

    pucTail = pucDnsPkt + uiPktLen;

    while (*pucData != 0)
    {
        uiLen = *pucData;
        if (uiLen > DNS_MAX_LABEL_LEN)
        {
            return BS_ERR;
        }

        if (uiDomainNameLen + uiLen > DNS_MAX_DOMAIN_NAME_LEN)
        {
            return BS_ERR;
        }

        pucData ++;
        if (pucData + uiLen > pucTail)
        {
            return BS_ERR;
        }
        
        memcpy(pcDst, pucData, uiLen);

        pcDst += uiLen;
        *pcDst = '.';
        pcDst ++;
        pucData += uiLen;

        uiDomainNameLen += (uiLen + 1);
    }

    if (pcDst != szDomainName)
    {
        pcDst --;
        *pcDst = '\0';
    }

    return BS_OK;
}

/* 返回主机序DNS Server地址. 这个函数应该拿到HostIpInfo模块去 */
UINT DNS_GetHostDnsServer()
{
    UINT uiIp = 0;
    
#ifdef IN_WINDOWS
    {
        FIXED_INFO * FixedInfo;
        ULONG ulOutBufLen;
        DWORD dwRetVal;

		FixedInfo = (FIXED_INFO *) GlobalAlloc( GPTR, sizeof( FIXED_INFO ) );
        ulOutBufLen = sizeof( FIXED_INFO );

        if( ERROR_BUFFER_OVERFLOW == GetNetworkParams( FixedInfo, &ulOutBufLen ) )
        {
            GlobalFree( FixedInfo );
            FixedInfo = (FIXED_INFO *) GlobalAlloc( GPTR, ulOutBufLen );
        }

        if ( dwRetVal = GetNetworkParams( FixedInfo, &ulOutBufLen ) == 0)
        {
            if (FixedInfo->DnsServerList.IpAddress.String[0] != '\0')
            {
                uiIp = Socket_NameToIpHost(FixedInfo->DnsServerList.IpAddress.String);
            }
        }

        GlobalFree( FixedInfo );
    }

#endif

    return uiIp;
}

CHAR * DNS_Header2String(IN VOID *dnspkt, IN int pktlen, OUT CHAR *info, IN UINT infosize)
{
    DNS_HEADER_S *dns = dnspkt;

    *info = 0;

    if (pktlen < sizeof(DNS_HEADER_S)) {
        return NULL;
    }

    snprintf(info, infosize,
            "\"ID\":%u,\"QR\":%u,\"opcode\":%u,\"AA\":%u,\"TC\":%u,"
            "\"RD\":%u,\"RA\":%u,\"rcode\":%u,"
            "\"question\":%u, \"answer_rr\":%u,"
            "\"authority_rr\":%u,\"additional_rr\":%u",
            ntohs(dns->usTransID), dns->usQR, dns->usOpcode, dns->usAA, dns->usTC,
            dns->usRD, dns->usRA, dns->usRcode,
            ntohs(dns->usQdCount), ntohs(dns->usAnCount),
            ntohs(dns->usNsCount), ntohs(dns->usArCount));

    return info;
}

