/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/ip_utl.h"
#include "utl/ip_string.h"
#include "utl/udp_utl.h"
#include "utl/tcp_utl.h"
#include "utl/vxlan_utl.h"
#include "utl/subcmd_utl.h"
#include "pcap.h"
#include "utl/pcap_file.h"

static int pcap_read(int argc, char **argv);
static int pcap_raw2pcap(int argc, char **argv);
static int pcap_ip2eth(int argc, char **argv);

static SUB_CMD_NODE_S g_pcap_subcmds[] = 
{
    {"read", pcap_read},
    {"raw2pcap", pcap_raw2pcap},
    {"ip2eth", pcap_ip2eth},
    {NULL, NULL}
};

static void pcap_help_read()
{
    printf("Usage: pcap read [OPTIONS] pcap_file\r\n");
    printf("Options:\r\n");
    printf("  -h,       Help\r\n");
    return;
}

static void pcap_help_raw2pcap()
{
    printf("Usage: pcap raw2pcap [OPTIONS] raw_file pcap_file\r\n");
    printf("Options:\r\n");
    printf("  -h,       Help\r\n");
    return;
}

static void pcap_help_ip2eth()
{
    printf("Usage: pcap ip2eth [OPTIONS] pcap_ip_file pcap_file \r\n");
    printf("Options:\r\n");
    printf("  -h,       Help\r\n");
    return;
}

static int pcap_dump_pkt(unsigned char *data, PCAP_PKT_HEADER_S *header)
{
    IP_HEAD_S *ip_header;
    IP_HEAD_S *new_ip_header;
    UDP_HEAD_S *udp_header;
    VXLAN_HEAD_S *vxlan_header;
    unsigned char *new_eth = NULL;
    int inner_pkt_type;
    int left_len;
    void *l4_header;
    char info[256];

    printf("******************************************************\n");

    ip_header = IP_GetIPHeader(data, header->cap_len, NET_PKT_TYPE_ETH);
    l4_header = (char*)ip_header + IP_HEAD_LEN(ip_header);

    IPString_IPHeader2String(ip_header, info, sizeof(info));
    printf("IP: {%s}\n", info);

    if (ip_header->ucProto == IPPROTO_TCP) {
        TCP_Header2String(l4_header, info, sizeof(info));
        printf("TCP: {%s}\n", info);
    }

    if (ip_header->ucProto == IPPROTO_UDP) {
        UDP_Header2String(l4_header, info, sizeof(info));
        printf("UDP: {%s}\n", info);
    }

    left_len = header->cap_len - ((char*)ip_header - (char*)data);
    udp_header = UDP_GetUDPHeader(ip_header, left_len, NET_PKT_TYPE_IP);
    if (NULL == udp_header) {
        return 0;
    }

    vxlan_header = VXLAN_GetVxlanHeader(ip_header, left_len, NET_PKT_TYPE_IP);
    if (NULL == vxlan_header) {
        return 0;
    }

    if ((udp_header->usDstPort != htons(4789))
            && (udp_header->usDstPort != htons(250))) {
        return 0;
    }

    if (! VXLAN_Valid(vxlan_header)) {
        return 0; 
    }

    VXLAN_Header2String(vxlan_header, info, sizeof(info));
    printf("VXLAN: {%s}\n", info);

    inner_pkt_type = VXLAN_GetInnerPktType(vxlan_header, 0);

    if (inner_pkt_type == VXLAN_NEXT_PROTOCOL_ETH) {
        new_eth = (void*)(vxlan_header + 1);
        new_ip_header = (void*)(new_eth + 14);
    } else if (inner_pkt_type == VXLAN_NEXT_PROTOCOL_IPv4) {
        new_ip_header = (void*)(vxlan_header + 1);
    } else {
        return 0;
    }

    IPString_IPHeader2String(new_ip_header, info, sizeof(info));
    printf("IP: {%s}\n", info);

    return 0;
}

static int pcap_readfile(FILE *fp)
{
    PCAPFILE_HEADER_S header;
    PCAP_PKT_HEADER_S pkt_header;
    unsigned char data[65535];

    if (0 != PCAPFILE_ReadHeader(fp, &header)) {
        return -1;
    }

    while (1) {
        if (0 != PCAPFILE_ReadPkt(fp, data, sizeof(data), &pkt_header)) {
            break;
        }

        pcap_dump_pkt(data, &pkt_header);
    }

    return 0;
}

static int pcap_trans_ip2eth(FILE *fp, FILE *fq)
{
    PCAPFILE_HEADER_S header;
    PCAP_PKT_HEADER_S pkt_header;
    unsigned char data[65535];

    if (0 != PCAPFILE_ReadHeader(fp, &header)) {
        return -1;
    }

    if (header.link_type != DLT_RAW) {
        return -1;
    }

    PCAPFILE_WriteHeader(fq, DLT_EN10MB);

    int len = sizeof(ETH_HEADER_S);
    memset(data, 0, len);
    ETH_HEADER_S *eth_header = (void*)data;
    eth_header->usProto = htons(ETH_P_IP);

    while (1) {
        if (0 != PCAPFILE_ReadPkt(fp, data+len, sizeof(data)-len, &pkt_header)) {
            break;
        }

        PCAPFILE_WritePkt(fq, data, &pkt_header.time,  pkt_header.cap_len + len, pkt_header.pkt_len + len);
    }

    return 0;
}

static int pcap_ip2eth(int argc, char **argv)
{
    int c;
    FILE *fp, *fq;

    if (argc < 2) {
        pcap_help_ip2eth();
        return -1;
    }

    while ((c = getopt(argc, argv, "h")) != -1) {
        switch (c) {
            case 'h':
                pcap_help_ip2eth();
                return 0;
                break;
            default:
                printf("Unknown option -%c\r\n", c);
                pcap_help_read();
                return -1;
        }
    }

    if (argc <= optind) {
        pcap_help_ip2eth();
        return -1;
    }

    fp = PCAPFILE_Open(argv[optind], "rb");
    if (NULL == fp) {
        printf("Can't open file %s\r\n", argv[optind]);
        return -1;
    }

    fq = PCAPFILE_Open(argv[optind + 1], "wb+");
    if (NULL == fq) {
        printf("Can't open file %s\r\n", argv[optind + 1]);
        return -1;
    }

    pcap_trans_ip2eth(fp, fq);

    PCAPFILE_Close(fp);
    PCAPFILE_Close(fq);

    return 0;
}
static int pcap_raw2pcap(int argc, char **argv)
{
    int c;
    FILE *fp, *fq;

    if (argc < 2) {
        pcap_help_raw2pcap();
        return -1;
    }

    while ((c = getopt(argc, argv, "h")) != -1) {
        switch (c) {
            case 'h':
                pcap_help_raw2pcap();
                return 0;
                break;
            default:
                printf("Unknown option -%c\r\n", c);
                pcap_help_read();
                return -1;
        }
    }

    if (argc <= optind) {
        pcap_help_raw2pcap();
        return -1;
    }

    fp = fopen(argv[optind], "rb");
    if (NULL == fp) {
        printf("Can't open file %s\r\n", argv[optind]);
        return -1;
    }

    fq = PCAPFILE_Open(argv[optind + 1], "rb");
    if (NULL == fq) {
        printf("Can't open file %s\r\n", argv[optind + 1]);
        return -1;
    }

    pcap_readfile(fq);

    fclose(fp);
    PCAPFILE_Close(fq);

    return 0;
}

int pcap_read(int argc, char **argv)
{
    int c;
    FILE *fp;

    if (argc < 2) {
        pcap_help_read();
        return -1;
    }

    while ((c = getopt(argc, argv, "h")) != -1) {
        switch (c) {
            case 'h':
                pcap_help_read();
                return 0;
                break;
            default:
                printf("Unknown option -%c\r\n", c);
                pcap_help_read();
                return -1;
        }
    }

    if (argc <= optind) {
        pcap_help_read();
        return -1;
    }

    fp = PCAPFILE_Open(argv[optind], "rb");
    if (NULL == fp) {
        printf("Can't open file %s\r\n", argv[optind]);
        return -1;
    }

    pcap_readfile(fp);

    PCAPFILE_Close(fp);

    return 0;
}

int main(int argc, char **argv)
{
    return SUBCMD_Do(g_pcap_subcmds, argc, argv);
}

