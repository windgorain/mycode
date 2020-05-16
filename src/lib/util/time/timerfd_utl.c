/*================================================================
*   Created by LiXingang: 2018.11.20
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/timerfd_utl.h"
#include "utl/socket_utl.h"

#if (defined IN_WINDOWS) || (defined IN_MAC)

static void _win_timerfd_Main(IN USER_HANDLE_S *pstUserHandle)
{
    int fd = HANDLE_UINT(pstUserHandle->ahUserHandle[0]);
    int interval = HANDLE_UINT(pstUserHandle->ahUserHandle[1]);

    for (;;) {
        Sleep(interval);
        Socket_Write(fd, "0", 1, 0);
    }
}

static int _os_timerfd_create(int interval)
{
    int fds[2];
    USER_HANDLE_S user_data;

    if (BS_OK != Socket_Pair(SOCK_STREAM, fds)) {
        return -1;
    }

    user_data.ahUserHandle[0] = UINT_HANDLE(fds[1]);
    user_data.ahUserHandle[1] = UINT_HANDLE(interval);

    THREAD_Create("timer_fd", NULL, _win_timerfd_Main, &user_data);

    return fds[0];
}

static void _os_timerfd_close(int fd)
{
    Socket_Close(fd);
}
#endif

#ifdef IN_LINUX
#include <sys/timerfd.h>
static int _os_timerfd_create(int interval)
{
    int timer_fd;
    struct itimerspec new_value;
    int sec = interval / 1000;
    int nsec = (interval % 1000) * 1000;

    new_value.it_value.tv_sec = sec;
    new_value.it_value.tv_nsec = nsec;
    new_value.it_interval.tv_sec = sec;
    new_value.it_interval.tv_nsec = nsec;

    timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timer_fd < 0) {
        return -1;
    }

    timerfd_settime(timer_fd, 0, &new_value, NULL);

    return 0;
}

static void _os_timerfd_close(int fd)
{
    close(fd);
}
#endif

int TimerFd_Create(int interval/* ms */)
{
    return _os_timerfd_create(interval);
}

void TimerFd_Close(int fd)
{
    _os_timerfd_close(fd);
}
