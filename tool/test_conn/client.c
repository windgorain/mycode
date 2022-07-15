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

static conn()
{
    int i;
    int val=1;
    
    printf("Start......\r\n");

    int sockfd = socket(AF_INET,SOCK_STREAM,0);
    if (0 > sockfd) {
        printf("socket err\r\n");
    }

    ioctl(sockfd, FIONBIO, &val);

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(10261);
    addr.sin_addr.s_addr = inet_addr("11.112.6.157");

    int keepAlive = 1;

    setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (void*)&keepAlive, sizeof(keepAlive));

    connect(sockfd,(saddrp)&addr,sizeof(addr));

    printf("Success......\r\n");
}

int main(int argc, char const *argv[])
{
    conn();

    while(1) {
        sleep(360);
    }

    return 0;
}


