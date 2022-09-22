/*================================================================
*   Created：2018.10.11
*   Description：
*
================================================================*/
#include "bs.h"
#include "utl/route_utl.h"
#include "utl/tun_utl.h"
#include "utl/ipcmd_utl.h"

#include "udp_utl.h"

static void usage(char *proc_name)
{
    printf("Usage: %s proxy_ip_list\r\n", proc_name);
}

int main(int argc, char **argv)
{
    int tun_fd, net_fd;
    char ifname[16];

    if(argc < 2) {
        usage(argv[0]);
        return -1;
    }

    tun_fd = TUN_Open(ifname, sizeof(ifname));
    net_fd = Socket_OpenUdp(0, htons(8888));

    if ((tun_fd < 0) || (net_fd < 0)) {
        return -1;
    }

    IPCMD_SetMtu(ifname, 1380);
    IPCMD_AddIP(ifname, inet_addr("172.16.1.2"), 0xffffffff,
            inet_addr("172.16.1.1"));

#if 0
    gw_ip = Route_GetDefaultGw();
    if (gw_ip != 0) {
        RouteCmd_Add(htonl(0x1e000000), 8, gw_ip);
    }
    RouteCmd_Add(htonl(0x80000000), 1, inet_addr("172.16.1.1"));
    RouteCmd_Add(htonl(0x00000000), 1, inet_addr("172.16.1.1"));
#endif

    TUNC_Run(tun_fd, net_fd, argv[1], htons(8888));

    return 0;
}
