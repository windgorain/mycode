/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"

#ifdef IN_UNIXLIKE
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#endif

#include "utl/tun_utl.h"
#include "utl/txt_utl.h"

#ifdef IN_MAC
#include <sys/kern_control.h>
#include <sys/kern_event.h>
#include<sys/uio.h>

#define UTUN_CONTROL_NAME "com.apple.net.utun_control"
#define UTUN_OPT_IFNAME 2

static int _tunos_Open(char *dev, int dev_name_size, int type)
{
    struct sockaddr_ctl addr;
    struct ctl_info info;
    int fd = -1;
    int err = 0;
    socklen_t len = dev_name_size;

    fd = socket (PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
    if (fd < 0) return fd;

    bzero(&info, sizeof (info));
    strncpy(info.ctl_name, UTUN_CONTROL_NAME, MAX_KCTL_NAME);

    err = ioctl(fd, CTLIOCGINFO, &info);
    if (err != 0){
        close(fd);
        return err;
    }

    addr.sc_len = sizeof(addr);
    addr.sc_family = AF_SYSTEM;
    addr.ss_sysaddr = AF_SYS_CONTROL;
    addr.sc_id = info.ctl_id;
    addr.sc_unit = 0;

    err = connect(fd, (struct sockaddr *)&addr, sizeof (addr));
    if (err != 0){
        close(fd);
        return err;
    }

    fcntl(fd, F_SETFD, FD_CLOEXEC);
    if (err != 0){
        close(fd);
        return err;
    }

    getsockopt(fd, SYSPROTO_CONTROL, UTUN_OPT_IFNAME, dev, &len);

    return fd;
}

static int _tunos_Read(IN int fd, OUT void *buf, IN int buf_size)
{
    struct iovec iv[2];
    unsigned int type;
    int nread;

    iv[0].iov_base = (char*)&type;
    iv[0].iov_len = sizeof(type);
    iv[1].iov_base = buf;
    iv[1].iov_len = buf_size;

    nread = readv(fd, iv, 2);
    if (nread <= 0) {
        return nread;
    }

    nread -= sizeof(type);
    if (nread <= 0) {
        return -1;
    }

    return nread;
}

static int _tunos_Write(IN int fd, IN void *buf, IN int len)
{
    unsigned int type;
    struct ip *iph;
    struct iovec iv[2];
 
 	iph = (struct ip *) buf;
    if(iph->ip_v == 6) {
        type = htonl(AF_INET6);
    } else {
		type = htonl(AF_INET);
    }

    iv[0].iov_base = (char *)&type;
    iv[0].iov_len  = sizeof(type);
	iv[1].iov_base = buf;
	iv[1].iov_len  = len;

    return writev(fd, iv, 2);
}

static int _tunos_SetNonblock(int fd)
{
    return fcntl(fd, F_SETFL, O_NONBLOCK);
}

#endif

#ifdef IN_LINUX
#include <linux/if_tun.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>

static int _tunos_Open(char *dev, int dev_name_size, int type)
{
    struct ifreq ifr;
    int fd, err;
    char *clonedev = "/dev/net/tun";

    if ((fd = open(clonedev, O_RDWR)) < 0) {
        return fd;
    }

    memset(&ifr, 0, sizeof(ifr));

    if (type == TUN_TYPE_TUN) {
        ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    } else {
        ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    }

    if ((dev != NULL) && (*dev != '\0')) {  
        strlcpy(ifr.ifr_name, dev, IFNAMSIZ);  
    } 

    if ((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0) {
        close(fd);
        return err;
    }

    if (dev != NULL) {
        strlcpy(dev, ifr.ifr_name, dev_name_size);
    }

    return fd;
}

#ifdef IFF_MULTI_QUEUE
static int _tunos_mque_Open(INOUT char *dev, int dev_name_size, IN int que_num, OUT int *fds)
{
    struct ifreq ifr;
    int fd, err;
    int i;

    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = IFF_TAP | IFF_NO_PI | IFF_MULTI_QUEUE;
    if ((dev != NULL) && (*dev != '\0')) {  
        strlcpy(ifr.ifr_name, dev, IFNAMSIZ);  
    } 

    for (i = 0; i < que_num; i++) {
        if ((fd = open("/dev/net/tun", O_RDWR)) < 0)
            goto err;
        err = ioctl(fd, TUNSETIFF, (void *)&ifr);
        if (err) {
            close(fd);
            goto err;
        }
        fds[i] = fd;
    }

    return 0;
err:
    for (--i; i >= 0; i--)
        close(fds[i]);
    return err;
}
#endif

static int _tunos_Read(IN int fd, OUT void *buf, IN int buf_size)
{
    return read(fd, buf, buf_size);
}

static int _tunos_Write(IN int fd, IN void *buf, IN int len)
{
    return write(fd, buf, len);
}

static int _tunos_SetNonblock(int fd)
{
    return fcntl(fd, F_SETFL, O_NONBLOCK);
}

#endif

#ifdef IN_WINDOWS


VNIC_HANDLE _tunos_Open(OUT char * dev_name, int dev_name_size, int type)
{
    VNIC_HANDLE hVnic;
    char *ifname;
    UINT uiStatus = 1;
    UINT uiLen;

    hVnic = VNIC_Dev_Open();
    if (hVnic == NULL) {
        return 0;
    }

    VNIC_Dev_SetTun(hVnic, inet_addr("172.16.1.2"), inet_addr("255.255.255.252"));
    VNIC_Ioctl (hVnic, TAP_WIN_IOCTL_SET_MEDIA_STATUS, (UCHAR*)&uiStatus, 4, (UCHAR*)&uiStatus, 4, &uiLen);

    if (dev_name != NULL) {
        ifname = VNIC_GetAdapterGuid(hVnic);
        strlcpy(dev_name, ifname, dev_name_size);
    }

    return hVnic;
}

static int _tunos_SetNonblock(VNIC_HANDLE hVnic)
{
    return 0;
}

static int _tunos_Read(VNIC_HANDLE hVnic, OUT void *buf, IN int buf_size)
{
    UINT nread;

    if (BS_OK != VNIC_Read(hVnic, BS_WAIT, 0, buf, buf_size, &nread)) {
        return -1;
    }

    return (int)nread;
}

static int _tunos_Write(VNIC_HANDLE hVnic, IN void *buf, IN int len)
{
    UINT write_len;

    if (BS_OK != VNIC_Write(hVnic, buf, len, &write_len)) {
        return -1;
    }

    return (int)write_len;
}

#endif

/*
 * dev_name: 允许为NULL
 */
TUN_FD TUN_Open(char *dev_name, int dev_name_size)
{
    return _tunos_Open(dev_name, dev_name_size, TUN_TYPE_TUN);
}

TUN_FD TAP_Open(char *dev_name, int dev_name_size)
{
    return _tunos_Open(dev_name, dev_name_size, TUN_TYPE_TAP);
}

int TUN_SetNonblock(TUN_FD fd)
{
    return _tunos_SetNonblock(fd);
}

int TUN_Read(IN TUN_FD fd, OUT void *buf, IN int buf_size)
{
    return _tunos_Read(fd, buf, buf_size);
}

int TUN_Write(IN TUN_FD fd, IN void *buf, IN int len)
{
    return _tunos_Write(fd, buf, len);
}

#ifdef IN_LINUX
#ifdef IFF_MULTI_QUEUE
int TUN_MQUE_Open(INOUT char *dev_name, int dev_name_size, IN int que_num, OUT int *fds)
{
    return _tunos_mque_Open(dev_name, dev_name_size, que_num, fds);
}
#endif
#endif

