/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

typedef struct sockaddr* saddrp;

int main(int argc, char const *argv[])
{
    struct sockaddr_in addr;
    int fd=socket(AF_INET,SOCK_STREAM,0);
    int val = 1;

    if(fd<0) {
        perror("socket");
        return 1;
    }

    addr.sin_family=AF_INET;
    socklen_t addr_len=sizeof(addr);
    addr.sin_addr.s_addr=0;
    addr.sin_port=htons(8888);

    int bind_ret=bind(fd,(struct sockaddr *)&addr,addr_len);
    if(bind_ret<0) {
        perror("bind");
        return 1;
    }

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void*)&val, sizeof(int));

    int listen_ret=listen(fd,50);
    if(listen_ret<0) {
        perror("listen");
        return 1;
    }

    while(1)
    {
        int listen_socket=accept(fd,(struct sockaddr *)&addr,&addr_len);
        if(listen_socket<0) {
            perror("accept");
            continue;
        }
    }

    return 0;
}
