/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2013-4-8
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/dns_utl.h"
#include "utl/bit_opt.h"

/* 遇到了cxxx, 获取offset  */
static int _dns_get_offset(char *name, int len)
{
    USHORT offset = 0;

    if (len < 2) {
        RETURN(BS_ERR);
    }

    MEM_Copy(&offset, name, sizeof(USHORT));
    offset = ntohs(offset);
    BIT_CLR(offset, 0xC000);

    return offset;
}

static int _dns_EachQuery(void *dns_header, UINT pkt_len,
        UCHAR *rrhead, UINT datalen, PF_DNS_WALK_RR func, void *ud)
{
    DNS_WALK_QR_S qr;
    int rrsize;
    UINT name_size = DNS_GetDnsNameSize(rrhead, datalen);

    rrsize = sizeof(DNS_QST_S) + name_size;
    if (rrsize > datalen) {
        return BS_ERR;
    }

    qr.dns_header = dns_header;
    qr.rrtype = DNS_RRTYPE_QUERY;
    qr.rr_data = rrhead;
    qr.rr_struct = rrhead + name_size;
    qr.rr_size = rrsize;
    qr.dns_pkt_len = pkt_len;
    qr.domain_size = name_size;

    func(&qr, ud);

    return rrsize;
}

static int _dns_EachRR(void *dns_header, UINT pkt_len,
        int rrtype, UCHAR *rrhead, UINT datalen, PF_DNS_WALK_RR func, void *ud)
{
    DNS_WALK_QR_S qr;
    int rrsize;
    DNS_RR_S *rr_struct;
    UINT name_size = DNS_GetDnsNameSize(rrhead, datalen);

    rrsize = sizeof(DNS_RR_S) + name_size;
    if (rrsize > datalen) {
        return BS_ERR;
    }

    rr_struct = (void*)(rrhead + name_size);
    rrsize += ntohs(rr_struct->usRDLen);
    if (rrsize > datalen) {
        return BS_ERR;
    }

    qr.dns_header = dns_header;
    qr.rrtype = rrtype;
    qr.rr_data = rrhead;
    qr.rr_struct = rr_struct;
    qr.rr_size = rrsize;
    qr.dns_pkt_len = pkt_len;
    qr.domain_size = name_size;

    func(&qr, ud);

    return rrsize;
}


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

/* 获得DNS的数字分格式的域名的长度(不展开),包含开头的第一个数字和最后的'\0' . 返回0表示出错 */
UINT DNS_GetDnsNameSize(IN UCHAR *pucData, IN UINT uiMaxSize)
{
    UINT uiDnsNameSize = 0;
    UINT uiLableLen;
    UCHAR *pucDataTmp = pucData;
    UINT uiDnsNameMaxSize;

    uiDnsNameMaxSize = MIN(uiMaxSize, DNS_MAX_DNS_NAME_SIZE);

    if (uiDnsNameMaxSize < 2) {
        return 0;
    }

    while (*pucDataTmp != 0) {
        if ((pucDataTmp[0] & 0xc0) == 0xc0) {
            uiDnsNameSize ++;
            break;
        }

        uiLableLen = *pucDataTmp;
        uiDnsNameSize += (uiLableLen + 1);

        if (uiDnsNameSize >= uiDnsNameMaxSize) {
            return 0;
        }

        pucDataTmp = pucData + uiDnsNameSize;
    }

    return uiDnsNameSize + 1;
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

/* 遍历Query和RR */
void DNS_WalkQR(DNS_HEADER_S *pstDnsHeader, UINT uiDataLen, PF_DNS_WALK_RR func, void *ud)
{
    UCHAR *rrhead;
    USHORT count;
    UINT reserved_len = uiDataLen;
    int rrsize;
    int i;

    if (uiDataLen <= sizeof(DNS_HEADER_S)) {
        return;
    }

    rrhead = (UCHAR*)(pstDnsHeader + 1);
    reserved_len -= sizeof(DNS_HEADER_S);

    count = ntohs(pstDnsHeader->usQdCount);
    for (i=0; i<count; i++) {
        rrsize = _dns_EachQuery(pstDnsHeader, uiDataLen, rrhead, reserved_len, func, ud);
        if (rrsize < 0) {
            return;
        }
        reserved_len -= rrsize;
        rrhead += rrsize;
    }

    count = ntohs(pstDnsHeader->usAnCount);
    for (i=0; i<count; i++) {
        rrsize = _dns_EachRR(pstDnsHeader, uiDataLen, DNS_RRTYPE_ANSWER, rrhead, reserved_len, func, ud);
        if (rrsize < 0) {
            return;
        }
        reserved_len -= rrsize;
        rrhead += rrsize;
    }

    count = ntohs(pstDnsHeader->usNsCount);
    for (i=0; i<count; i++) {
        rrsize = _dns_EachRR(pstDnsHeader, uiDataLen, DNS_RRTYPE_AUTHORITY, rrhead, reserved_len, func, ud);
        if (rrsize < 0) {
            return;
        }
        reserved_len -= rrsize;
        rrhead += rrsize;
    }

    count = ntohs(pstDnsHeader->usArCount);
    for (i=0; i<count; i++) {
        rrsize = _dns_EachRR(pstDnsHeader, uiDataLen, DNS_RRTYPE_ADDITIONAL, rrhead, reserved_len, func, ud);
        if (rrsize < 0) {
            return;
        }
        reserved_len -= rrsize;
        rrhead += rrsize;
    }
}

/* 返回构造之后的报文长度 */
UINT DNS_BuildQuestPacket(CHAR *pcDomainName, USHORT usType, OUT DNS_PKT_S *pstPkt)
{
    UINT uiLen;
    CHAR *pucData;
    DNS_HEADER_S *pstDnsHeader;
    UINT uiRemainSize;
    DNS_QST_S *pstQst;
    static USHORT g_usDnsTID; /* DNS标识 */

    if (strlen(pcDomainName) > DNS_MAX_DOMAIN_NAME_LEN) {
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

int DNS_GetRRDomainName(UCHAR *dns_pkt, int pkt_len, void *rr, OUT char domain_name[DNS_MAX_DOMAIN_NAME_LEN + 1])
{
    UCHAR *pucData = rr;
    UINT uiLen;
    CHAR *pcDst = domain_name;
    UCHAR *pucTail;
    UINT uiDomainNameLen = 0;

    domain_name[0] = '\0';

    pucTail = dns_pkt + pkt_len;

    if (pucData >= pucTail) {
        RETURN(BS_BAD_PARA);
    }

    while (*pucData != 0) {
        if (BIT_MATCH(*pucData, 0xC0)) { /* 处理压缩情况 */
            int offset = _dns_get_offset((char*)pucData, pucTail - pucData);
            if (offset < 0) {
                RETURN(BS_ERR);
            }
            if (offset >= (pucData - dns_pkt)) { /* 只能指向之前的位置 */
                RETURN(BS_ERR);
            }
            pucData = dns_pkt + offset;
            continue;
        }

        uiLen = *pucData;
        if (uiLen > DNS_MAX_LABEL_LEN) {
            RETURN(BS_ERR);
        }

        if (uiDomainNameLen + uiLen > DNS_MAX_DOMAIN_NAME_LEN) {
            RETURN(BS_ERR);
        }

        pucData ++;
        if (pucData + uiLen > pucTail) {
            RETURN(BS_OUT_OF_RANGE);
        }
        
        memcpy(pcDst, pucData, uiLen);

        pcDst += uiLen;
        *pcDst = '.';
        pcDst ++;
        pucData += uiLen;

        uiDomainNameLen += (uiLen + 1);
    }

    if (pcDst != domain_name) {
        pcDst --;
        *pcDst = '\0';
    }

    return 0;
}

int DNS_GetQueryNameByPacket(UCHAR *pucDnsPkt, int pkt_len, OUT char szDomainName[DNS_MAX_DOMAIN_NAME_LEN + 1])
{
    UCHAR *pucData;

    szDomainName[0] = '\0';

    if (pkt_len <= sizeof(DNS_HEADER_S)) {
        return BS_ERR;
    }

    pucData = pucDnsPkt + sizeof(DNS_HEADER_S);

    return DNS_GetRRDomainName(pucDnsPkt, pkt_len, pucData, szDomainName);
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

