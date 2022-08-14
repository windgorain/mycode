/*================================================================
*   Created：2018.10.12 LiXingang All rights reserved.
*   Description：
*
================================================================*/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/kern_event.h>
#include <sys/socket.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <sys/kern_control.h>
#include <ctype.h>
#include <fcntl.h>

#define UTUN_CONTROL_NAME "com.apple.net.utun_control"
#define UTUN_OPT_IFNAME 2

int UTUN_Open()
{
    struct sockaddr_ctl addr;
    struct ctl_info info;
    int fd = -1;
    int err = 0;

    fd = socket (PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
    if (fd < 0) {
        return fd;
    }

    bzero(&info, sizeof (info));
    strncpy(info.ctl_name, UTUN_CONTROL_NAME, MAX_KCTL_NAME);

    err = ioctl(fd, CTLIOCGINFO, &info);
    if (err != 0) {
        close(fd);
        return err;
    }

    addr.sc_len = sizeof(addr);
    addr.sc_family = AF_SYSTEM;
    addr.ss_sysaddr = AF_SYS_CONTROL;
    addr.sc_id = info.ctl_id;
    addr.sc_unit = 0;

    err = connect(fd, (struct sockaddr *)&addr, sizeof (addr));
    if (err != 0) {
        close(fd);
        return err;
    }

    fcntl(fd, F_SETFD, FD_CLOEXEC);

    return fd;
}

int UTUN_OpenByID(int id)
{
    int err;
    int fd;
    struct sockaddr_ctl addr;
    struct ctl_info info;

    fd = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
    if (fd < 0) {
        return fd;
    }
    memset(&info, 0, sizeof (info));
    strncpy(info.ctl_name, UTUN_CONTROL_NAME, strlen(UTUN_CONTROL_NAME));
    err = ioctl(fd, CTLIOCGINFO, &info);
    if (err < 0) {
        close(fd);
        return err;
    }

    addr.sc_id = info.ctl_id;
    addr.sc_len = sizeof(addr);
    addr.sc_family = AF_SYSTEM;
    addr.ss_sysaddr = AF_SYS_CONTROL;
    addr.sc_unit = id + 1; // utunX where X is sc.sc_unit -1

    err = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    if (err < 0) {
        // this utun is in use
        close(fd);
        return err;
    }

    fcntl(fd, F_SETFD, FD_CLOEXEC);

    return fd;
}

int UTUN_GetName(int fd, char *ifname, socklen_t *len)
{
    return getsockopt(fd, SYSPROTO_CONTROL, UTUN_OPT_IFNAME, ifname, len);
}

int UTUN_SetNoneBlock(int fd)
{
    return fcntl(fd, F_SETFL, O_NONBLOCK);
}

