#include "bs.h"

#include "utl/net.h"
#include "utl/eth_utl.h"
#include "utl/ip_utl.h"
#include "utl/data2hex_utl.h"
#include "utl/tcp_utl.h"
#include "utl/in_checksum.h"

/* 返回网络序的校验和 */
USHORT TCP_CheckSum
(
    IN UCHAR *pucBuf,
	IN UINT uiLen,
	IN UCHAR *pucSrcIp,
	IN UCHAR *pucDstIp
)
{
    USHORT usCheckSum = 0;

    /* 1. 计算TCP头和数据的校验和 */
    usCheckSum = IN_CHKSUM_AddRaw(usCheckSum, pucBuf, uiLen);

    /* 2. 计算伪首部的源IP的校验和 */
    usCheckSum = IN_CHKSUM_AddRaw(usCheckSum, pucSrcIp, 4);

    /* 3. 计算伪首部的目的IP的校验和 */
    usCheckSum = IN_CHKSUM_AddRaw(usCheckSum, pucDstIp, 4);

    /* 4. 计算伪首部的类型和长度字段校验和 */
    usCheckSum = IN_CHKSUM_AddRawWord(usCheckSum, IP_PROTO_TCP);
    usCheckSum = IN_CHKSUM_AddRawWord(usCheckSum, uiLen);

    return IN_CHKSUM_Wrap(usCheckSum);
}

TCP_HEAD_S * TCP_GetTcpHeader(IN UCHAR *pucData, IN UINT uiDataLen, IN NET_PKT_TYPE_E enPktType)
{
    IP46_HEAD_S pstIpHeader = {0};
    TCP_HEAD_S *pstTcpHeader = NULL;
    NET_PKT_TYPE_E enPktTypeTmp = enPktType;
    UINT uiHeadLen = 0;

    if (enPktTypeTmp != NET_PKT_TYPE_TCP) {
        if (0 != IP46_GetIPHeader(&pstIpHeader, pucData, uiDataLen, enPktType)) {
            return NULL;
        }

        if (pstIpHeader.family == ETH_P_IP) {
            if (pstIpHeader.iph.ip4->ucProto != IP_PROTO_TCP) {
                return NULL;
            }
            uiHeadLen = (((UCHAR*)pstIpHeader.iph.ip4) - pucData) + IP_HEAD_LEN(pstIpHeader.iph.ip4);
        } else if (pstIpHeader.family == ETH_P_IP6) {
            if (pstIpHeader.iph.ip6->next != IP_PROTO_TCP)
            {
                return NULL;
            }
            uiHeadLen = (((UCHAR*)pstIpHeader.iph.ip6) - pucData) + IP6_HDR_LEN;
        }

        enPktTypeTmp = NET_PKT_TYPE_TCP;
    }

    if (enPktTypeTmp == NET_PKT_TYPE_TCP)
    {
        /* 长度不够TCP头的长度则返回NULL */
        if ((uiHeadLen + sizeof(TCP_HEAD_S)) > uiDataLen)
        {
            return NULL;
        }

        pstTcpHeader = (TCP_HEAD_S*)(pucData + uiHeadLen);
    }

    return pstTcpHeader;
}

char * TCP_GetFlagsString(IN UCHAR ucFlags, char *info)
{
    char *ptr = info;

    if (ucFlags & TCP_FLAG_FIN) {
        *ptr++ = 'F';
    }
    if (ucFlags & TCP_FLAG_SYN) {
        *ptr++ = 'S';
    }
    if (ucFlags & TCP_FLAG_RST) {
        *ptr++ = 'R';
    }
    if (ucFlags & TCP_FLAG_PUSH) {
        *ptr++ = 'P';
    }
    if (ucFlags & TCP_FLAG_ACK) {
        *ptr++ = 'A';
    }
    if (ucFlags & TCP_FLAG_URG) {
        *ptr++ = 'U';
    }
    if (ucFlags & TCP_FLAG_ECE) {
        *ptr++ = 'E';
    }
    if (ucFlags & TCP_FLAG_CWR) {
        *ptr++ = 'C';
    }

    *ptr = '\0';

    return info;
}

void TCP_GetOptInfo(TCP_HEAD_S *tcph, TCP_OPT_INFO_S *opt_info)
{
    int iOptLen = TCP_HEAD_LEN(tcph) - 20;
    UCHAR *data;
    UCHAR kind;
    UCHAR len;
    int nop_count = 0;
    int eol_count = 0;
    UINT *puidata;

    memset(opt_info, 0, sizeof(TCP_OPT_INFO_S));

    data = (void*)(tcph + 1);
    while (iOptLen > 0) {
        kind = *data;
        if ((kind == TCP_OPT_NOP) || (kind == TCP_OPT_EOL)) {
            len = 1;
        } else if (iOptLen >= 2) {
            len = *(data + 1);
            if ((len < 2) || (iOptLen < len)) {
                break;
            }
        } else {
            break;
        }

        switch (kind) {
            case TCP_OPT_MSS: {
                if (len == 4) {
                    USHORT *mss = (void*)(data + 2);
                    opt_info->mss = ntohs(*mss);
                }
                break;
            }
            case TCP_OPT_WIN: {
                if (len == 3) {
                    opt_info->wscale = *(data + 2);
                }
                break;
            }
            case TCP_OPT_SACK: {
                opt_info->sack = 1;
                break;
            }
            case TCP_OPT_TIME: {
                if (len == 10) {
                    puidata = (void*)(data + 2);
                    opt_info->timestamp = ntohl(*puidata);
                    puidata = (void*)(data + 6);
                    opt_info->timestamp_echo = ntohl(*puidata);
                }
                break;
            }
            case TCP_OPT_NOP: {
                nop_count ++;
                break;
            }
            case TCP_OPT_EOL: {
                eol_count ++;
                break;
            }
        }

        data += len;
        iOptLen -= len;
    }

    opt_info->nop_count = nop_count;
    opt_info->eol_count = eol_count;

}

int TCP_GetOptString(TCP_HEAD_S *tcph, char *buf, int buf_len)
{
    TCP_OPT_INFO_S stInfo;
    char *ptr = buf;

    TCP_GetOptInfo(tcph, &stInfo);

    int offset = 0, left_len = buf_len;
    if (stInfo.mss > 0) {
        offset = snprintf(ptr, left_len,  ",\"tcp_opt_mss\":%u", stInfo.mss);
        ptr += offset;
        left_len -= offset;
    }
    
    if (stInfo.wscale > 0) {
        offset = snprintf(ptr, left_len, ",\"tcp_opt_wscale\":%u", stInfo.wscale);
        ptr += offset;
        left_len -= offset;
    }

    if (stInfo.sack > 0) {
        offset = snprintf(ptr, left_len, ",\"tcp_opt_sack\":1");
        ptr += offset;
        left_len -= offset;
    }

    if (stInfo.timestamp > 0) {
        offset = snprintf(ptr, left_len, ",\"tcp_opt_timestamp\":%u", stInfo.timestamp);
        ptr += offset;
        left_len -= offset;
        offset = snprintf(ptr, left_len, ",\"tcp_opt_timestamp_echo\":%u", stInfo.timestamp_echo);
        ptr += offset;
        left_len -= offset;
    }

    if (stInfo.nop_count > 0) {
        offset = snprintf(ptr, left_len, ",\"tcp_opt_nop\":%u", stInfo.nop_count);
        ptr += offset;
        left_len -= offset;
    }

    if (stInfo.eol_count > 0) {
        offset = snprintf(ptr, left_len, ",\"tcp_opt_eol\":%u", stInfo.eol_count);
        ptr += offset;
        left_len -= offset;
    }

    return ptr - buf;
}


CHAR * TCP_Header2String(IN VOID *tcp, OUT CHAR *info, IN UINT infosize)
{
    TCP_HEAD_S *tcph = tcp;
    char szFlags[16];
    char option[1024];

    TCP_GetOptString(tcph, option, sizeof(option));

    snprintf(info, infosize,
            "\"sport\":%u,\"dport\":%u,\"sequence\":%u, \"ack\":%u,"
            "\"tcp_head_len\":%u,\"tcp_flags\":\"%s\",\"tcp_wsize\":%u,"
            "\"tcp_chksum\":\"%02x\",\"tcp_urg\":%u"
            "%s",
            ntohs(tcph->usSrcPort), ntohs(tcph->usDstPort), ntohl(tcph->ulSequence), ntohl(tcph->ulAckSequence),
            TCP_HEAD_LEN(tcph), TCP_GetFlagsString(tcph->ucFlag, szFlags), ntohs(tcph->usWindow),
            ntohs(tcph->usCrc), ntohs(tcph->usUrg), option);

    return info;
}

CHAR * TCP_Header2Hex(IN VOID *tcp, OUT CHAR *hex)
{
    TCP_HEAD_S *tcph = tcp;
    int len = TCP_HEAD_LEN(tcph);

    DH_Data2HexString(tcp, len, hex);

    return hex;
}

