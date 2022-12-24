/*================================================================
*   Created by LiXingang: 2018.11.19
*   Description: 
*
================================================================*/
#ifndef _IPCMD_UTL_H
#define _IPCMD_UTL_H
#ifdef __cplusplus
extern "C"
{
#endif

/* 在win中ifname为adapter guid*/
BS_STATUS IPCMD_AddIP(char *ifname, UINT ip/*net order*/, UINT mask/*net order*/, UINT peer/*permit 0*/);
BS_STATUS IPCMD_AddDns(char *ifname, UINT dnsip, UINT index);
BS_STATUS IPCMD_DelDns(char *ifname, UINT dnsip);
BS_STATUS IPCMD_SetDns(char *ifname, UINT dnsip);
BS_STATUS IPCMD_SetMtu(char *ifname, UINT mtu);


#ifdef __cplusplus
}
#endif
#endif //IPCMD_UTL_H_
