/*================================================================
*   Created：2018.10.15
*   Description：
*
================================================================*/
#ifndef _SNC_TUNNEL_H
#define _SNC_TUNNEL_H

#ifdef __cplusplus
extern "C"
{
#endif

int SNC_TUN_Run(VNIC_HANDLE tun_fd, int net_fd, char *dst_ip, unsigned short dst_port);

#ifdef __cplusplus
}
#endif
#endif 
