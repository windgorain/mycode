/*================================================================
*   Created：2018.10.26
*   Description：
*
================================================================*/
#include "bs.h"
#include <stdlib.h>
#include <stdio.h>
#include "utl/vnic_lib.h"
#include "utl/vnic_tap.h"
#include "utl/route_utl.h"
#include "utl/ipcmd_utl.h"
#include "udp_utl.h"
#include "snc_tunnel.h"

static void _statement()
{
    printf("******************************************\r\n");
    printf("By LiXingang\r\n");
    printf("Version: 1.0\r\n");
    printf("Build Date: %s %s\r\n", __DATE__, __TIME__);
    printf("******************************************\r\n");
}

static void usage(char *proc_name)
{
    printf("Usage:\r\n %s proxy_ip_list\r\n", proc_name);
}

int main(int argc, char **argv)
{
    int net_fd;
    unsigned int gw_ip;
    VNIC_HANDLE hVnic;
    char *ifname;
    UINT uiStatus = 1;
    UINT uiLen;

    if(argc < 2) {
        usage(argv[0]);
        return -1;
    }

    _statement();

    hVnic = VNIC_Dev_Open();
    net_fd = Socket_OpenUdp(0, htons(443));

    if ((hVnic == 0) || (net_fd < 0)) {
        return -1;
    }

    VNIC_Dev_SetTun(hVnic, inet_addr("172.16.1.2"), inet_addr("255.255.255.252"));

    ifname = VNIC_GetAdapterGuid(hVnic);

    IPCMD_SetMtu(ifname, 1380);
    IPCMD_AddIP(ifname, inet_addr("172.16.1.2"), inet_addr("255.255.255.252"), inet_addr("172.16.1.1"));
    VNIC_Ioctl (hVnic, TAP_WIN_IOCTL_SET_MEDIA_STATUS, (UCHAR*)&uiStatus, 4, (UCHAR*)&uiStatus, 4, &uiLen);
    Sleep(1000);

    gw_ip = Route_GetDefaultGw();
    if (gw_ip != 0) {
        RouteCmd_Add(htonl(0x1e000000), 8, gw_ip);
        RouteCmd_Add(htonl(0x0a000000), 8, gw_ip);
        RouteCmd_Add(htonl(0x0b000000), 8, gw_ip);
    }
    RouteCmd_Add(htonl(0x80000000), 1, inet_addr("172.16.1.1"));
    RouteCmd_Add(htonl(0x00000000), 1, inet_addr("172.16.1.1"));

    SNC_TUN_Run(hVnic, net_fd, argv[1], htons(443));

    return 0;
}

