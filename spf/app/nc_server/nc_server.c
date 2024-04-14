/*********************************************************
*   Copyright (C) LiXingang
*
********************************************************/
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>         
#include <sys/socket.h>
#include <arpa/inet.h>
#include "utl/ulc_user.h"

enum {
    STATE_STOPED = 0,
    STATE_RUNNING,
    STATE_STOPPING
};

static volatile int g_nc_server_need_state = 0;

static int _nc_server_do(int s)
{
    char buf[128];
    int len;
    struct timeval timeout={1,0}; /* 1s */

    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    while(g_nc_server_need_state == STATE_RUNNING) { 
        len = recv(s, buf, sizeof(buf) -1, 0);
        if ((len < 0) && ((errno == EAGAIN) || (errno == EINTR))) {
            continue;
        }

        if (len <= 0) {
            return -1;
        }

        buf[len] = '\0';
        printf("%s", buf);
    }

    return 0;
}

static void _nc_server_accept(int fd)
{
    int s;
    struct timeval timeout={1,0}; /* 1s */

    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    g_nc_server_need_state = STATE_RUNNING;

    while(g_nc_server_need_state == STATE_RUNNING) { 
        s = accept(fd, NULL, NULL);
        if ((s < 0) && ((errno == EAGAIN) || (errno == EINTR))) {
            continue;
        }

        if (s < 0) {
            break;
        }

        _nc_server_do(s);

        close(s);
    }
}

static void _nc_server_main(void *arg)
{
    int reuse=1;

    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd < 0) {
        printf("Can't create socket \n");
        return;
    }

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(int));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(5000);
    server_addr.sin_addr.s_addr= htonl(0x7f000001);
    if (bind(fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Can't bind socket \n");
        close(fd);
        return;
    }

	if (listen(fd, 10) < 0) {
        printf("Can't Listen \n");
        close(fd);
        return;
	}

    _nc_server_accept(fd);

    close(fd);

    g_nc_server_need_state = STATE_STOPED;
}

SEC(".spf/init")
int main()
{
    pthread_t tid;
    pthread_create(&tid, NULL, _nc_server_main, NULL);
    return 0;
}

SEC(".spf/finit")
int finit()
{
    if (g_nc_server_need_state == STATE_STOPED) {
        return 0;
    }

    g_nc_server_need_state = STATE_STOPPING;

    return -1;
}
