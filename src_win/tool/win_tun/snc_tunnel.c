#include "bs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "utl/vnic_lib.h"
#include "utl/vnic_tap.h"
#include "snc_tunnel.h"

#define SN_TUN_OPT_TYPE_KEEPLIVE 1  
#define SN_TUN_OPT_TYPE_PKT      4  
#define SN_TUN_OPT_TYPE_USERINFO 6  

typedef struct
{
    UCHAR version;
    UCHAR reserved1; 
    USHORT reserved2; 
}SN_TUN_HEAD_S;

typedef struct
{
    UCHAR type;
    UCHAR flag;
    USHORT length; 
}SN_TUN_OPT_S;

#define SNTUN_HEAD_LEN (sizeof(SN_TUN_HEAD_S) + sizeof(SN_TUN_OPT_S))

static char * g_user_info = "uid:12345,mac:0101-aaaa-bbbb,os:win7sp1,localtion:BeiJing.Office"; 
static VNIC_HANDLE g_hVnic = 0;
static int g_netfd = 0;
static unsigned int g_proxy_ip = 0;
static unsigned short g_proxy_port = 0;

static int _snc_tun_GetUserinfoSize()
{
    int len = strlen(g_user_info);

    
    len = ((len + 3) >> 2) << 2;

    return len;
}


static int _snc_tun_Tun2Net(VNIC_HANDLE tun_fd, int net_fd, struct sockaddr_in *dst)
{
    int nread, nwrite;
    SN_TUN_HEAD_S *tun_head;
    SN_TUN_OPT_S *pkt_opt, *user_info_opt;
    unsigned char buffer[2048];
    unsigned char *read_buf;
    unsigned int read_buf_size;
    int user_info_size = _snc_tun_GetUserinfoSize();

    read_buf =  buffer + SNTUN_HEAD_LEN + user_info_size + sizeof(SN_TUN_OPT_S);
    read_buf_size = sizeof(buffer) - SNTUN_HEAD_LEN - user_info_size - sizeof(SN_TUN_OPT_S);

    VNIC_Read(tun_fd, 1, 0, read_buf, read_buf_size, &nread);
    if (nread <= 0) {
        return 0;
    }

    if((read_buf[0] >> 4) != 4) {
        return 0;
    }


    printf("tun to net %d\r\n", nread);

    tun_head = (SN_TUN_HEAD_S*)buffer;
    user_info_opt = (SN_TUN_OPT_S*)(void*)(tun_head+1);
    pkt_opt = (SN_TUN_OPT_S*) (((char*)user_info_opt) + user_info_size + sizeof(SN_TUN_OPT_S));

    tun_head->version = 1; 
    tun_head->reserved1 = tun_head->reserved2 = 0;

    user_info_opt->type = SN_TUN_OPT_TYPE_USERINFO;
    user_info_opt->flag = 0;
    user_info_opt->length = htons(user_info_size);
    memset(user_info_opt+1, 0, user_info_size);
    memcpy(user_info_opt+1, g_user_info, strlen(g_user_info));

    pkt_opt->type = SN_TUN_OPT_TYPE_PKT;
    pkt_opt->flag = 0;
    pkt_opt->length = htons(nread);

    nwrite = nread + SNTUN_HEAD_LEN + user_info_size + sizeof(SN_TUN_OPT_S);

    return sendto(net_fd, buffer, nwrite, 0, (struct sockaddr*)dst, sizeof(struct sockaddr_in));
}

static int _snc_tun_Net2Tun(VNIC_HANDLE tun_fd, int net_fd)
{
    unsigned char buffer[2048];
    unsigned char *data;
    struct sockaddr_in addr;
    int addr_len = sizeof(struct sockaddr_in);
    int nrecv;
    unsigned int len;

    nrecv = recvfrom(net_fd, buffer, sizeof(buffer), 0, (struct sockaddr*)&addr, &addr_len);
    if (nrecv <= SNTUN_HEAD_LEN) {
        return -1;
    }

    data = (buffer + SNTUN_HEAD_LEN);

    return VNIC_Write(tun_fd, data, nrecv - SNTUN_HEAD_LEN, &len);
}

static unsigned int _snc_tun_GetNearIP(char *proxy_ip_list)
{
    

    
    return inet_addr(proxy_ip_list);
}

static DWORD WINAPI ThreadTun2Net(LPVOID lpParameter)
{
    struct sockaddr_in addr = {0};

    addr.sin_addr.s_addr = g_proxy_ip;
    addr.sin_family = AF_INET;
    addr.sin_port = g_proxy_port;

    while(1) {
        _snc_tun_Tun2Net(g_hVnic, g_netfd, &addr);
    }

    return 0;
}

int SNC_TUN_Run(VNIC_HANDLE tun_fd, int net_fd, char *proxy_ip_list, unsigned short dst_port)
{
    g_hVnic = tun_fd;
    g_netfd = net_fd;
    g_proxy_port = dst_port;
    g_proxy_ip = _snc_tun_GetNearIP(proxy_ip_list);
    
    CreateThread(NULL,0,ThreadTun2Net,NULL,0,NULL);
    while(1) {
        _snc_tun_Net2Tun(tun_fd, net_fd);
    }
}
