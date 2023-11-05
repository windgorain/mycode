/*================================================================
*   Created：2018.06.04
*   Description：
*
================================================================*/
#include "bs.h"

#include "utl/eth_utl.h"
#include "utl/tcp_utl.h"
#include "utl/stream2ip.h"

#define STREAM2IP_MAX_IP_LEN 1500
#define STREAM2IP_MAX_TCP_PAYLOAD_LEN  (STREAM2IP_MAX_IP_LEN - sizeof(struct s2ip_tcp_hdr) - sizeof(struct s2ip_ipv4_hdr) - sizeof(ETH_HEADER_S))


struct s2ip_ipv4_hdr {
	unsigned char version_ihl;		
	unsigned char type_of_service;	
	unsigned short total_length;		
	unsigned short packet_id;		    
	unsigned short fragment_offset;	
	unsigned char time_to_live;		
	unsigned char next_proto_id;		
	unsigned short hdr_checksum;		
	unsigned int src_addr;		    
	unsigned int dst_addr;		    
} __attribute__((__packed__));

struct s2ip_tcp_hdr {
    unsigned short src_port;  
    unsigned short dst_port;  
    unsigned int sent_seq;  
    unsigned int recv_ack;  
    unsigned char rsvd:4;
    unsigned char hlen:4;
    unsigned char tcp_flags; 
    unsigned short rx_win;    
    unsigned short cksum;     
    unsigned short tcp_urp;   
} __attribute__((__packed__));

static inline unsigned short s2ip_get_16b_sum(void *ptr, unsigned short nr)
{
    unsigned short *ptr16 = ptr;
    unsigned int sum = 0;
    while (nr > 1)
    {
        sum +=*ptr16;
        nr -= sizeof(unsigned short);
        ptr16++;
        if (sum > 0xffff)
            sum -= 0xffff;
    }

    
    if (nr)
        sum += *((unsigned char*)ptr16);

    sum = ((sum & 0xffff0000) >> 16) + (sum & 0xffff);
    sum &= 0x0ffff;
    return (unsigned short)sum;
}

static inline void s2ip_fill_ip_hdr(S2IP_S *ctrl, struct s2ip_ipv4_hdr *ip, unsigned short total_len)
{
    static unsigned short packet_id = 0;
    unsigned short cksum;

    packet_id ++;

    ip->version_ihl = 0x45;
	ip->total_length = htons(total_len);
	ip->type_of_service = 0x00;
	ip->packet_id = packet_id;
	ip->fragment_offset = htons(0x4000);
	ip->time_to_live = 0xff;
	ip->next_proto_id = ctrl->protocol;
	ip->src_addr = ctrl->sip;
	ip->dst_addr = ctrl->dip;
	ip->hdr_checksum = 0;
    cksum = s2ip_get_16b_sum(ip, (ip->version_ihl & 0xf) * 4);
    ip->hdr_checksum = (unsigned short)((cksum == 0xffff)?cksum:~cksum);
}

static inline void s2ip_fill_tcp_hdr(S2IP_S *ctrl, struct s2ip_tcp_hdr *tcp,
        unsigned char flags)
{
    tcp->src_port = ctrl->sport;
    tcp->dst_port = ctrl->dport;
    tcp->sent_seq = htonl(ctrl->seq);
    tcp->recv_ack = htonl(ctrl->ack);
    tcp->rsvd = 0;
    tcp->hlen = 5;
    tcp->tcp_flags = flags;
    tcp->rx_win = htons(65535);
    tcp->tcp_urp = 0;
    tcp->cksum = 0;
    if (flags & (TCP_FLAG_SYN | TCP_FLAG_FIN)) {
        ctrl->seq ++;
    }
}


void S2IP_Init(OUT S2IP_S *ctrl, UINT sip, UINT dip, USHORT sport, USHORT dport)
{
    ctrl->sip = sip;
    ctrl->dip = dip;
    ctrl->sport = sport;
    ctrl->dport = dport;
    ctrl->protocol = IP_PROTO_TCP;
}

void S2IP_SetMac(OUT S2IP_S *ctrl, MAC_ADDR_S *smac, MAC_ADDR_S *dmac)
{
    ctrl->smac = *smac;
    ctrl->dmac = *dmac;
}


void S2IP_Switch(S2IP_S *ctrl)
{
    unsigned int addrtmp = ctrl->dip;
    ctrl->dip = ctrl->sip;
    ctrl->sip = addrtmp;

    unsigned short porttmp = ctrl->dport;
    ctrl->dport = ctrl->sport;
    ctrl->sport = porttmp;

    unsigned int sn = ctrl->seq;
    ctrl->seq = ctrl->ack;
    ctrl->ack = sn;
}


int S2IP_Hsk(S2IP_S *ctrl, PF_S2IP_OUTPUT output_func, void *user_handle)
{
    unsigned char buf[256];
    struct s2ip_ipv4_hdr *ip;
    struct s2ip_tcp_hdr *tcp_hdr;
    int total_len;
    int ip_total_len;
    ETH_HEADER_S *eth;

    eth = (void*)buf;
    ip = (void*)(eth + 1);
    tcp_hdr = (void*)(ip + 1);

    eth->stSMac = ctrl->smac;
    eth->stDMac = ctrl->dmac;
    eth->usProto = htons(ETH_P_IP);

    
    ip_total_len = sizeof(struct s2ip_tcp_hdr) + sizeof(struct s2ip_ipv4_hdr);
    total_len = ip_total_len + sizeof(ETH_HEADER_S);

    
    s2ip_fill_ip_hdr(ctrl, ip, ip_total_len);
    s2ip_fill_tcp_hdr(ctrl, tcp_hdr, 0x2);
    output_func(buf, total_len, user_handle);

    
    S2IP_Switch(ctrl);
    s2ip_fill_ip_hdr(ctrl, ip, ip_total_len);
    s2ip_fill_tcp_hdr(ctrl, tcp_hdr, 0x12);
    output_func(buf, total_len, user_handle);

    
    S2IP_Switch(ctrl);
    s2ip_fill_ip_hdr(ctrl, ip, ip_total_len);
    s2ip_fill_tcp_hdr(ctrl, tcp_hdr, 0x10);
    output_func(buf, total_len, user_handle);

    return 0;
}


int S2IP_Data(S2IP_S *ctrl, void *data, int data_len, BOOL_T ack, PF_S2IP_OUTPUT output_func, void *user_handle)
{
    unsigned char buf[STREAM2IP_MAX_IP_LEN];
    char *ptr = data;
    int left_len = data_len;
    int copy_len;
    struct s2ip_ipv4_hdr *ip;
    struct s2ip_tcp_hdr *tcp_hdr;
    void *tcp_payload;
    int ip_total_len;
    ETH_HEADER_S *eth;

    eth = (void*)buf;
    ip = (void*)(eth + 1);
    tcp_hdr = (void*)(ip + 1);
    tcp_payload = (void*)(tcp_hdr + 1);

    eth->stSMac = ctrl->smac;
    eth->stDMac = ctrl->dmac;
    eth->usProto = htons(ETH_P_IP);

    while (left_len > 0) {
        copy_len = MIN(STREAM2IP_MAX_TCP_PAYLOAD_LEN, (unsigned int)left_len);
        memcpy(tcp_payload, ptr, copy_len);
        ip_total_len = sizeof(struct s2ip_tcp_hdr) + sizeof(struct s2ip_ipv4_hdr) + copy_len;
        s2ip_fill_ip_hdr(ctrl, ip, ip_total_len);
        s2ip_fill_tcp_hdr(ctrl, tcp_hdr, 0x10);
        output_func(buf, ip_total_len + sizeof(ETH_HEADER_S), user_handle);

        ptr += copy_len;
        left_len -= copy_len;
        ctrl->seq += copy_len;
    }

    
    if (ack) {
        ip_total_len = sizeof(struct s2ip_tcp_hdr) + sizeof(struct s2ip_ipv4_hdr);

        S2IP_Switch(ctrl);
        s2ip_fill_ip_hdr(ctrl, ip, ip_total_len);
        s2ip_fill_tcp_hdr(ctrl, tcp_hdr, 0x10);
        output_func(buf, ip_total_len + sizeof(ETH_HEADER_S), user_handle);
        S2IP_Switch(ctrl);
    }

    return 0;
}


int S2IP_Bye(S2IP_S *ctrl, PF_S2IP_OUTPUT output_func, void *user_handle)
{
    unsigned char buf[256];
    struct s2ip_ipv4_hdr *ip;
    struct s2ip_tcp_hdr *tcp_hdr;
    ETH_HEADER_S *eth;
    int ip_total_len;

    eth = (void*)buf;
    ip = (void*)(eth + 1);
    tcp_hdr = (void*)(ip + 1);

    
    ip_total_len = sizeof(struct s2ip_tcp_hdr) + sizeof(struct s2ip_ipv4_hdr);

    s2ip_fill_ip_hdr(ctrl, ip, ip_total_len);
    s2ip_fill_tcp_hdr(ctrl, tcp_hdr, 0x11);
    output_func(buf, ip_total_len + sizeof(ETH_HEADER_S), user_handle);

    
    S2IP_Switch(ctrl);
    s2ip_fill_ip_hdr(ctrl, ip, ip_total_len);
    s2ip_fill_tcp_hdr(ctrl, tcp_hdr, 0x11);
    output_func(buf, ip_total_len + sizeof(ETH_HEADER_S), user_handle);
    S2IP_Switch(ctrl);

    return 0;
}

