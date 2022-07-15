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

int Socket_Bind(int iSocketId, unsigned int ulIp/* 网络序 */, unsigned short usPort/* 网络序 */)
{
    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(struct sockaddr_in));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = usPort;
    server_addr.sin_addr.s_addr= ulIp;

    if (bind(iSocketId, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        return -1;
    }

    return 0;
}


static conn(unsigned int sip)
{
    int i;
    int val=1;
    
    printf("Start......\r\n");

    for (i=0; i<60000; i++) {
        int sockfd = socket(AF_INET,SOCK_STREAM,0);
        if (0 > sockfd) {
            printf("socket err\r\n");
        }

        ioctl(sockfd, FIONBIO, &val);

        Socket_Bind(sockfd, sip, 0);

        struct sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(8888);
        addr.sin_addr.s_addr = inet_addr("202.118.1.2");

        connect(sockfd,(saddrp)&addr,sizeof(addr));
        //usleep(1);
    }

    printf("Success......\r\n");
}

int main(int argc, char const *argv[])
{
    conn(inet_addr("30.1.1.171"));
    conn(inet_addr("30.1.1.172"));
    conn(inet_addr("30.1.1.173"));
    conn(inet_addr("30.1.1.174"));
    conn(inet_addr("30.1.1.175"));

    while(1) {
        sleep(100);
    }

    return 0;
}


