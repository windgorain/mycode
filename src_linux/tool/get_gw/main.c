#include <stdio.h>
#include <arpa/inet.h>
#include <linux/rtnetlink.h>
#include <net/if.h> 
#include <stdlib.h> 
#include <string.h>


#define BUFSIZE 8192


struct route_info{      
    u_int dstAddr;
    u_int srcAddr;
    u_int gateWay;
    char ifName[IF_NAMESIZE];
}

int readNlSock(int sockFd, char *bufPtr, int seqNum, int pId)     
{       
    struct nlmsghdr *nlHdr;
    int readLen = 0, msgLen = 0;

    do
    {
        //收到内核的应答
        if((readLen = recv(sockFd, bufPtr, BUFSIZE - msgLen, 0)) < 0)
        {
            perror("SOCK READ: ");
            return -1;
        }

        nlHdr = (struct nlmsghdr *)bufPtr;

        //检查header是否有效
        if((NLMSG_OK(nlHdr, readLen) == 0) || (nlHdr->nlmsg_type == NLMSG_ERROR))
        {
            perror("Error in recieved packet");
            return -1;
        }

        if(nlHdr->nlmsg_type == NLMSG_DONE)
        {
            break;
        }
        else
        {
            bufPtr += readLen;
            msgLen += readLen;
        }

        if((nlHdr->nlmsg_flags & NLM_F_MULTI) == 0)
        {
            break;
        }

    }while((nlHdr->nlmsg_seq != seqNum) || (nlHdr->nlmsg_pid != pId));

    return msgLen;
}


//分析返回的路由信息
void parseRoutes(struct nlmsghdr *nlHdr, struct route_info *rtInfo,char *gateway)
{
    struct rtmsg *rtMsg;
    struct rtattr *rtAttr;
    int rtLen;
    char *tempBuf = NULL;
    struct in_addr dst;
    struct in_addr gate;
    tempBuf = (char *)malloc(100);
    rtMsg = (struct rtmsg *)NLMSG_DATA(nlHdr);

    // If the route is not for AF_INET or does not belong to main routing table
    //then return.
    if((rtMsg->rtm_family != AF_INET) || (rtMsg->rtm_table != RT_TABLE_MAIN))
    {
        return;
    }

    rtAttr = (struct rtattr *)RTM_RTA(rtMsg);
    rtLen = RTM_PAYLOAD(nlHdr);

    for(;RTA_OK(rtAttr,rtLen);rtAttr = RTA_NEXT(rtAttr,rtLen))
    {
        switch(rtAttr->rta_type)
        {
            case RTA_OIF:
                if_indextoname(*(int *)RTA_DATA(rtAttr), rtInfo->ifName);
                break;
            case RTA_GATEWAY:
                rtInfo->gateWay = *(u_int *)RTA_DATA(rtAttr);
                break;
            case RTA_PREFSRC:
                rtInfo->srcAddr = *(u_int *)RTA_DATA(rtAttr);
                break;
            case RTA_DST:
                rtInfo->dstAddr = *(u_int *)RTA_DATA(rtAttr);
                break;
        }
    }

    dst.s_addr = rtInfo->dstAddr;
    if(strstr((char *)inet_ntoa(dst), "0.0.0.0"))
    {
        printf("oif:%s",rtInfo->ifName);
        gate.s_addr = rtInfo->gateWay;
        sprintf(gateway, (char *)inet_ntoa(gate));
        printf("gw%s\n",gateway);
        gate.s_addr = rtInfo->srcAddr;
        printf("src:%s\n",(char *)inet_ntoa(gate));
        gate.s_addr = rtInfo->dstAddr;
        printf("dst:%s\n",(char *)inet_ntoa(gate));
    }

    free(tempBuf);
    return;
}


int get_gateway(char *gateway)
{
    struct nlmsghdr *nlMsg;
    struct rtmsg *rtMsg;
    struct route_info *rtInfo;
    char msgBuf[BUFSIZE];
    int sock, len, msgSeq = 0;

    if((sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0)
    {
        perror("Socket Creation: ");
        return -1;
    }

    memset(msgBuf, 0, BUFSIZE);
    nlMsg = (struct nlmsghdr *)msgBuf;
    rtMsg = (struct rtmsg *)NLMSG_DATA(nlMsg);
    nlMsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg)); // Length of message.
    nlMsg->nlmsg_type = RTM_GETROUTE; // Get the routes from kernel routing table .
    nlMsg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST; // The message is a request for dump.
    nlMsg->nlmsg_seq = msgSeq++; // Sequence of the message packet.
    nlMsg->nlmsg_pid = getpid(); // PID of process sending the request.

    if(send(sock, nlMsg, nlMsg->nlmsg_len, 0) < 0)
    {
        printf("Write To Socket Failed…\n");
        return -1;
    }


    if((len = readNlSock(sock, msgBuf, msgSeq, getpid())) < 0)
    {
        printf("Read From Socket Failed…\n");
        return -1;
    }

    rtInfo = (struct route_info *)malloc(sizeof(struct route_info));

    for(;NLMSG_OK(nlMsg,len);nlMsg = NLMSG_NEXT(nlMsg,len))
    {
        memset(rtInfo, 0, sizeof(struct route_info));
        parseRoutes(nlMsg, rtInfo, gateway);
    }

    free(rtInfo);
    close(sock);
    return 0;
}


int main()     
{         
    char buff[256];
    get_gateway(buff);
    printf("gw = %s\n",buff);
    return 0;
}  
