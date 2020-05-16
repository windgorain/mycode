
#include "bs.h"

#include "utl/dns_service.h"


#define DNS_SERVICE_NS_NAME "ns1.test.com"

/*
    OK: 成功
    ERR: 失败
    NO_SUCH: 未找到对应的域名
*/
BS_STATUS DNS_Service_PktInput
(
    IN DNS_SERVICE_HANDLE hDnsService,
    IN UCHAR *pucRequest,
    IN UINT uiRequestLen,
    OUT DNS_PKT_S *pstReply,
    OUT UINT *puiReplyLen
)
{
    CHAR szDomainName[DNS_MAX_DOMAIN_NAME_LEN + 1];
    UINT uiRemainLen;
    UINT uiLen;
    DNS_HEADER_S *pstDnsHeader;
    UINT uiDataLen;
    DNS_SERVICE_INFO_S stInfo;

    if (uiRequestLen > sizeof(DNS_PKT_S))
    {
        return BS_ERR;
    }
    
    if (BS_OK != DNS_GetDomainNameByPacket(pucRequest, uiRequestLen, szDomainName))
    {
        return BS_ERR;
    }

    if (BS_OK != DNS_Service_GetInfoByName(hDnsService, szDomainName, &stInfo))
    {
        return BS_NO_SUCH;
    }

    if (0 == stInfo.uiIP)
    {
        /* TODO : 应该回应Failed */
        return BS_NO_SUCH;
    }

    MEM_Copy(pstReply->aucData, pucRequest, uiRequestLen);
    
    pstDnsHeader = (DNS_HEADER_S*)(pstReply->aucData);
    pstDnsHeader->usQR = 1;
    pstDnsHeader->usRA = 1;
    pstDnsHeader->usAA = 1;

    uiRemainLen = sizeof(DNS_PKT_S) - uiRequestLen;
    uiDataLen = uiRequestLen;

    /* Answer */
    {
        pstDnsHeader->usAnCount = htons(1);
        uiLen = DNS_BuildRRCompress(sizeof(DNS_HEADER_S), DNS_TYPE_A, DNS_CLASS_IN, 60, 4,
                    (UCHAR*)&stInfo.uiIP, pstReply->aucData + uiDataLen, uiRemainLen);
        if (uiLen == 0)
        {
            return BS_ERR;
        }

        uiDataLen += uiLen;
        uiRemainLen -= uiLen;
    }

#if 0
    /* Authoritative */
    {
        pstDnsHeader->usNsCount = htons(1);
        uiNsLen = DNS_DomainName2DnsName(DNS_SERVICE_NS_NAME, szDnsName, sizeof(szDnsName));
        if (uiNsLen == 0)
        {
            return BS_ERR;
        }

        uiLen = DNS_BuildRR(stInfo.szAuthorName, DNS_TYPE_NS, DNS_CLASS_IN, 60,
            uiNsLen, szDnsName, pstReply->aucData + uiDataLen, uiRemainLen);
        if (uiLen == 0)
        {
            return BS_ERR;
        }

        uiNsNameOffset = uiDataLen + uiLen - uiNsLen;
        uiDataLen += uiLen;
        uiRemainLen -= uiLen;
    }

    /* Additional */
    {
        uiIP = htonl(0x0a0efffe);
        pstDnsHeader->usArCount = htons(1);
        uiLen = DNS_BuildRRCompress(uiNsNameOffset, DNS_TYPE_A, DNS_CLASS_IN, 60,
            4, (UCHAR*)&uiIP, pstReply->aucData + uiDataLen, uiRemainLen);
        if (uiLen == 0)
        {
            return BS_ERR;
        }
        uiDataLen += uiLen;
        uiRemainLen -= uiLen;
    }
#endif

    *puiReplyLen = uiDataLen;

    return BS_OK;
}



