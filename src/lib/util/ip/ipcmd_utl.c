/*================================================================
*   Created：2018.10.14 LiXingang All rights reserved.
*   Description：为接口通过cmd配置IP信息
*
================================================================*/
#include "bs.h"

#include "utl/txt_utl.h"

#ifdef IN_WINDOWS
#include "utl/sys_utl.h"
#include "utl/process_utl.h"
#include "utl/my_ip_helper.h"

static char * _oswin_AdapterGuid2If(char *adapter_guid, char *ifname, int ifname_size)
{
    SYS_OS_VER_E eVer;
    UINT uiIndex;

    eVer = SYS_GetOsVer();

    if (eVer < SYS_OS_VER_WIN_VISTA) {
        return adapter_guid;
    }

    if (BS_OK != My_IP_Helper_GetAdapterIndex(adapter_guid, &uiIndex)) {
        return NULL;
    }

    scnprintf(ifname, ifname_size, "%d", uiIndex);

    return ifname;
}

static BS_STATUS _os_ipcmd_AddIP(char *adapter_guid, UINT ip, UINT mask, UINT peer)
{
    CHAR szCmd[256];
    char ifname[32];
    UINT ip_setted = 0;
    UINT mask_setted = 0;
    UINT count = 0;
    unsigned char *cip = (void*)&ip;
    unsigned char *cmask = (void*)&mask;
    unsigned char *cpeer = (void*)&peer;

    scnprintf(szCmd, sizeof(szCmd),
            "netsh interface ip set address name=%s source=static addr=%u.%u.%u.%u mask=%u.%u.%u.%u",
            _oswin_AdapterGuid2If(adapter_guid, ifname, sizeof(ifname)),
            cip[0], cip[1], cip[2], cip[3], cmask[0], cmask[1], cmask[2], cmask[3]);

    PROCESS_CreateByFile(szCmd, NULL, PROCESS_FLAG_WAIT_FOR_OVER | PROCESS_FLAG_HIDE);

    
    while (count < 100) {
        My_IP_Helper_GetIPAddress(adapter_guid, &ip_setted, &mask_setted);
        if ((ip_setted == ip) && (mask_setted == mask)) {
            break;
        }
        Sleep(200);
        count ++;
    }

    return BS_OK;
}

static inline BS_STATUS _os_ipcmd_AddDns(char *adapter_guid, UINT uiDns, UINT uiIndex)
{
    CHAR szCmd[256];
    char ifname[32];

    BS_Snprintf(szCmd, sizeof(szCmd), "netsh interface ip add dns name=%s addr=%pI4 index=%d",
            _oswin_AdapterGuid2If(adapter_guid, ifname, sizeof(ifname)), &uiDns, uiIndex);

    PROCESS_CreateByFile(szCmd, NULL, PROCESS_FLAG_WAIT_FOR_OVER | PROCESS_FLAG_HIDE);

    return BS_OK;
}

static inline BS_STATUS _os_ipcmd_DelDns(char *adapter_guid, UINT uiDns)
{
    CHAR szCmd[256];
    char ifname[32];

    BS_Snprintf(szCmd, sizeof(szCmd), "netsh interface ip del dns name=%s addr=%pI4",
            _oswin_AdapterGuid2If(adapter_guid, ifname, sizeof(ifname)), &uiDns);

    PROCESS_CreateByFile(szCmd, NULL, PROCESS_FLAG_HIDE);

    return BS_OK;
}

static inline BS_STATUS _os_ipcmd_SetDns(char *adapter_guid, UINT uiDns)
{
    CHAR szCmd[256];
    char ifname[32];

    scnprintf(szCmd, sizeof(szCmd),
            "netsh interface ip set dns name=%s addr=%pI4 source=static",
            _oswin_AdapterGuid2If(adapter_guid, ifname, sizeof(ifname)), &uiDns);

    PROCESS_CreateByFile(szCmd, NULL, PROCESS_FLAG_WAIT_FOR_OVER | PROCESS_FLAG_HIDE);

    return BS_OK;
}

static inline BS_STATUS _os_ipcmd_SetMtu(char *adapter_guid, UINT mtu)
{
    char cmd[256];
    char ifname[32];

    scnprintf(cmd, sizeof(cmd), "netsh interface ipv4 set subinterface %s mtu=%d",
            _oswin_AdapterGuid2If(adapter_guid, ifname, sizeof(ifname)), mtu);
    PROCESS_CreateByFile(cmd, NULL, PROCESS_FLAG_WAIT_FOR_OVER | PROCESS_FLAG_HIDE);

    return BS_OK;
}

#endif

#ifdef IN_LINUX
static BS_STATUS _os_ipcmd_AddIP(char *ifname, UINT ip, UINT mask, UINT peer)
{
    char cmd[256];
    unsigned char *cip = (void*)&ip;
    unsigned char *cmask = (void*)&mask;
    unsigned char *cpeer = (void*)&peer;

    if (peer == 0) {
    scnprintf(cmd, sizeof(cmd), "ifconfig %s %u.%u.%u.%u netmask %u.%u.%u.%u",
		ifname, cip[0], cip[1], cip[2], cip[3], cmask[0], cmask[1], cmask[2], cmask[3]);
    } else {
    scnprintf(cmd, sizeof(cmd), "ifconfig %s %u.%u.%u.%u netmask %u.%u.%u.%u pointopoint %u.%u.%u.%u",
		ifname, cip[0], cip[1], cip[2], cip[3], cmask[0], cmask[1], cmask[2], cmask[3], cpeer[0], cpeer[1], cpeer[2], cpeer[3]);
    }
    
    if (system(cmd) < 0) {
        RETURN(BS_ERR);
    }

    return BS_OK;
}

static inline BS_STATUS _os_ipcmd_AddDns(char *ifname, UINT uiDns, UINT uiIndex)
{
    BS_DBGASSERT(0);
    RETURN(BS_NOT_SUPPORT);
}

static inline BS_STATUS _os_ipcmd_DelDns(char *ifname, UINT uiDns)
{
    BS_DBGASSERT(0);
    RETURN(BS_NOT_SUPPORT);
}

static inline BS_STATUS _os_ipcmd_SetDns(char *ifname, UINT uiDns)
{
    BS_DBGASSERT(0);
    RETURN(BS_NOT_SUPPORT);
}

static inline BS_STATUS _os_ipcmd_SetMtu(char *ifname, UINT mtu)
{
    char cmd[256];

    scnprintf(cmd, sizeof(cmd), "ifconfig %s mtu %d", ifname, mtu);
    if (system(cmd) < 0) {
        RETURN(BS_ERR);
    }
    return BS_OK;
}

#endif

#ifdef IN_MAC
static BS_STATUS _os_ipcmd_AddIP(char *ifname, UINT ip, UINT mask, UINT peer)
{
    char cmd[256];
    unsigned char *cip = (void*)&ip;
    unsigned char *cmask = (void*)&mask;
    unsigned char *cpeer = (void*)&peer;

    if (peer == 0) {
        scnprintf(cmd, sizeof(cmd), "ifconfig %s %u.%u.%u.%u netmask %u.%u.%u.%u",
                ifname, cip[0], cip[1], cip[2], cip[3], cmask[0], cmask[1], cmask[2], cmask[3]);
    } else {
        scnprintf(cmd, sizeof(cmd), "ifconfig %s %u.%u.%u.%u %u.%u.%u.%u netmask %u.%u.%u.%u up",
                ifname, cip[0], cip[1], cip[2], cip[3],
                cpeer[0], cpeer[1], cpeer[2], cpeer[3],
                cmask[0], cmask[1], cmask[2], cmask[3]);
    }

    system(cmd);

    return BS_OK;
}

static inline BS_STATUS _os_ipcmd_AddDns(char *ifname, UINT uiDns, UINT uiIndex)
{
    BS_DBGASSERT(0);
    RETURN(BS_NOT_SUPPORT);
}

static inline BS_STATUS _os_ipcmd_DelDns(char *ifname, UINT uiDns)
{
    BS_DBGASSERT(0);
    RETURN(BS_NOT_SUPPORT);
}

static inline BS_STATUS _os_ipcmd_SetDns(char *ifname, UINT uiDns)
{
    BS_DBGASSERT(0);
    RETURN(BS_NOT_SUPPORT);
}

static inline BS_STATUS _os_ipcmd_SetMtu(char *ifname, UINT mtu)
{
    char cmd[256];

    scnprintf(cmd, sizeof(cmd), "ifconfig %s mtu %d", ifname, mtu);
    system(cmd);
    return BS_OK;
}

#endif


BS_STATUS IPCMD_AddIP(char *ifname, UINT ip, UINT mask, UINT peer)
{
    return _os_ipcmd_AddIP(ifname, ip, mask, peer);
}

BS_STATUS IPCMD_AddDns(char *ifname, UINT dnsip, UINT index)
{
    return _os_ipcmd_AddDns(ifname, dnsip, index);
}

BS_STATUS IPCMD_DelDns(char *ifname, UINT dnsip)
{
    return _os_ipcmd_DelDns(ifname, dnsip);
}

BS_STATUS IPCMD_SetDns(char *ifname, UINT dnsip)
{
    return _os_ipcmd_SetDns(ifname, dnsip);
}

BS_STATUS IPCMD_SetMtu(char *ifname, UINT mtu)
{
    return _os_ipcmd_SetMtu(ifname, mtu);
}

