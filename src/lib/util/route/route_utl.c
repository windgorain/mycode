/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/route_utl.h"

#ifdef IN_WINDOWS
#include "utl/sys_utl.h"
#include "utl/process_utl.h"
#include "utl/my_ip_helper.h"

static UINT _os_route_GetDefaultGw()
{
    return My_IP_Helper_GetDftGateway();
}

static int _os_routecmd_Add(unsigned int dst, unsigned int prefix_len, unsigned int nexthop, int isStatic)
{
    char cmd[256];
    unsigned char *dst_char = (void*)&dst;
    unsigned char *nexthop_char = (void*)&nexthop;
    char *route_str = "route";

    if (isStatic) {
        route_str = "route -p";
    }

    snprintf(cmd, sizeof(cmd), "%s add %d.%d.%d.%d/%d %d.%d.%d.%d METRIC 5",
            route_str, dst_char[0], dst_char[1], dst_char[2], dst_char[3], prefix_len,
            nexthop_char[0], nexthop_char[1], nexthop_char[2], nexthop_char[3]);
    PROCESS_CreateByFile(cmd, NULL, PROCESS_FLAG_WAIT_FOR_OVER | PROCESS_FLAG_HIDE);

    snprintf(cmd, sizeof(cmd), "%s change %d.%d.%d.%d/%d %d.%d.%d.%d METRIC 5",
            route_str, dst_char[0], dst_char[1], dst_char[2], dst_char[3], prefix_len,
            nexthop_char[0], nexthop_char[1], nexthop_char[2], nexthop_char[3]);

    PROCESS_CreateByFile(cmd, NULL, PROCESS_FLAG_WAIT_FOR_OVER | PROCESS_FLAG_HIDE);

    return BS_OK;
}

static int _os_routecmd_Del(unsigned int dst, unsigned int prefix_len)
{
    char cmd[256];
    unsigned char *dst_char = (void*)&dst;

    snprintf(cmd, sizeof(cmd), "route delete %d.%d.%d.%d/%d",
            dst_char[0], dst_char[1], dst_char[2], dst_char[3], prefix_len);
    PROCESS_CreateByFile(cmd, NULL, PROCESS_FLAG_WAIT_FOR_OVER | PROCESS_FLAG_HIDE);
}

#endif

#ifdef IN_LINUX
#include <net/if.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#define BUFSIZE 8192

struct route_info{
    u_int dstAddr;
    u_int srcAddr;
    u_int gateWay;
    char ifName[IF_NAMESIZE];
};

int readNlSock(int sockFd, char *bufPtr, int seqNum, int pId)
{
    struct nlmsghdr *nlHdr;
    int readLen = 0, msgLen = 0;

    do
    {
        
        if((readLen = recv(sockFd, bufPtr, BUFSIZE - msgLen, 0)) < 0)
        {
            perror("SOCK READ: ");
            return -1;
        }

        nlHdr = (struct nlmsghdr *)bufPtr;

        
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


static unsigned int parseRoutes(struct nlmsghdr *nlHdr, struct route_info *rtInfo)
{
    struct rtmsg *rtMsg;
    struct rtattr *rtAttr;
    int rtLen;
    struct in_addr dst;
    rtMsg = (struct rtmsg *)NLMSG_DATA(nlHdr);

    
    
    if((rtMsg->rtm_family != AF_INET) || (rtMsg->rtm_table != RT_TABLE_MAIN)) {
        return 0;
    }

    rtAttr = (struct rtattr *)RTM_RTA(rtMsg);
    rtLen = RTM_PAYLOAD(nlHdr);

    for(;RTA_OK(rtAttr,rtLen);rtAttr = RTA_NEXT(rtAttr,rtLen)) {
        switch(rtAttr->rta_type) {
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
    if(strstr((char *)inet_ntoa(dst), "0.0.0.0")) {
        return rtInfo->gateWay;
    }

    return 0;
}

static UINT _os_route_GetDefaultGw()
{
    struct nlmsghdr *nlMsg;
    struct route_info *rtInfo;
    char msgBuf[BUFSIZE];
    int sock, len, msgSeq = 0;
    UINT gw = 0;

    if((sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0) {
        return 0;
    }

    memset(msgBuf, 0, BUFSIZE);
    nlMsg = (struct nlmsghdr *)msgBuf;
    nlMsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg)); 
    nlMsg->nlmsg_type = RTM_GETROUTE; 
    nlMsg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST; 
    nlMsg->nlmsg_seq = msgSeq++; 
    nlMsg->nlmsg_pid = getpid(); 

    if(send(sock, nlMsg, nlMsg->nlmsg_len, 0) < 0) {
        close(sock);
        return 0;
    }

    if((len = readNlSock(sock, msgBuf, msgSeq, getpid())) < 0) {
        close(sock);
        return 0;
    }

    rtInfo = (struct route_info *)malloc(sizeof(struct route_info));

    for(;NLMSG_OK(nlMsg,len);nlMsg = NLMSG_NEXT(nlMsg,len))
    {
        memset(rtInfo, 0, sizeof(struct route_info));
        gw = parseRoutes(nlMsg, rtInfo);
        if (gw != 0) {
            break;
        }
    }

    free(rtInfo);
    close(sock);

    return gw;
}

static int _os_routecmd_Add(unsigned int dst, unsigned int prefix_len, unsigned int nexthop, int isStatic)
{
    char cmd[256];
    unsigned char *dst_char = (void*)&dst;
    unsigned char *nexthop_char = (void*)&nexthop;

    snprintf(cmd, sizeof(cmd), "ip route add %d.%d.%d.%d/%d via  %d.%d.%d.%d",
            dst_char[0], dst_char[1], dst_char[2], dst_char[3], prefix_len,
            nexthop_char[0], nexthop_char[1], nexthop_char[2], nexthop_char[3]);
    if (system(cmd)) {
        RETURN(BS_ERR);
    }

    return BS_OK;
}


static int _os_routecmd_Del(unsigned int dst, unsigned int prefix_len)
{
    char cmd[256];
    unsigned char *dst_char = (void*)&dst;

    snprintf(cmd, sizeof(cmd), "ip route del %d.%d.%d.%d/%d",
            dst_char[0], dst_char[1], dst_char[2], dst_char[3], prefix_len);
    if (system(cmd)) {
        RETURN(BS_ERR);
    }

    return BS_OK;
}

#endif

#ifdef IN_MAC

#include <sys/param.h>
#include <sys/sysctl.h>
#include <arpa/inet.h>
#include <net/route.h>

#ifndef SA_SIZE
# define SA_SIZE(sa)                        \
    (  (!(sa) || ((struct sockaddr *)(sa))->sa_len == 0) ?  \
           sizeof(long)     :               \
           1 + ( (((struct sockaddr *)(sa))->sa_len - 1) | (sizeof(long) - 1) ) )
#endif

#define	SA	struct sockaddr


#define ROUNDUP(a, size) (((a) & ((size)-1)) ? (1 + ((a) | ((size)-1))) : (a))


#define NEXT_SA(ap)	ap = (SA *) \
	((caddr_t) ap + (ap->sa_len ? ROUNDUP(ap->sa_len, sizeof (u_long)) : \
									sizeof(u_long)))

void
get_rtaddrs(int addrs, SA *sa, SA **rti_info)
{
	int		i;

	for (i = 0; i < RTAX_MAX; i++) {
		if (addrs & (1 << i)) {
			rti_info[i] = sa;
			NEXT_SA(sa);
		} else
			rti_info[i] = NULL;
	}
}

static int _os_route_GetMask(SA *sa, socklen_t salen, OUT UINT *mask)
{
    unsigned char *ch = (void*)mask;
    unsigned char *ptr = (void*)&sa->sa_data[2];

    *mask = 0;

    if (sa->sa_len == 0) {
        return 0;
    }

    if (sa->sa_len == 5) {
        ch[0] = *ptr; 
        return 0;
    }

    if (sa->sa_len == 6) {
        ch[0] = *ptr;
        ch[1] = *(ptr+1);
        return 0;
    }
    if (sa->sa_len == 7) {
        ch[0] = *ptr;
        ch[1] = *(ptr+1);
        ch[2] = *(ptr+2);
        return 0;
    }

    if (sa->sa_len == 8) {
        ch[0] = *ptr;
        ch[1] = *(ptr+1);
        ch[2] = *(ptr+2);
        ch[3] = *(ptr+3);
        return 0;
    }

    return -1;
}

static UINT _os_route_GetDefaultGw()
{
    size_t needed;
    int mib[6];
    char *buf, *next, *lim;
    struct rt_msghdr *rtm;
	struct sockaddr *sa, *rti_info[RTAX_MAX];
    struct sockaddr_in *sockin;
    UINT uiGw = 0;
    UINT mask;

    mib[0] = CTL_NET;
    mib[1] = PF_ROUTE;
    mib[2] = 0;
    mib[3] = 0;
    mib[4] = NET_RT_DUMP;
    mib[5] = 0;

    if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0) {
        return 0;
    }

    if ((buf = (char *)malloc(needed)) == NULL) {
        return 0;
    }

    if (sysctl(mib, 6, buf, &needed, NULL, 0) < 0) {
        free(buf);
        return 0;
    }

    lim  = buf + needed;
    for (next = buf; next < lim; next += rtm->rtm_msglen) {
        rtm = (struct rt_msghdr *)next;
        sa = (struct sockaddr *)(rtm + 1);

		get_rtaddrs(rtm->rtm_addrs, sa, rti_info);

        sa = rti_info[RTAX_NETMASK];

        if (sa == NULL) {
            continue;
        }

        if (0 != _os_route_GetMask(sa, sa->sa_len, &mask)) {
            continue;
        }

        if (mask != 0) {
            continue;
        }

        sa = rti_info[RTAX_GATEWAY];
        if (sa == NULL) {
            continue;
        }

        sockin = (struct sockaddr_in *)sa;
        uiGw = sockin->sin_addr.s_addr;
        break;
    }

    free(buf);

    return uiGw;
}

static int _os_routecmd_Add(unsigned int dst, unsigned int prefix_len, unsigned int nexthop, int isStatic)
{
    char cmd[256];
    unsigned char *dst_char = (void*)&dst;
    unsigned char *nexthop_char = (void*)&nexthop;

    snprintf(cmd, sizeof(cmd), "route -n add -net %d.%d.%d.%d/%d %d.%d.%d.%d > /dev/null 2>&1",
            dst_char[0], dst_char[1], dst_char[2], dst_char[3], prefix_len,
            nexthop_char[0], nexthop_char[1], nexthop_char[2], nexthop_char[3]);
    if (system(cmd)) {
        RETURN(BS_ERR);
    }
    snprintf(cmd, sizeof(cmd), "route -n change -net %d.%d.%d.%d/%d %d.%d.%d.%d > /dev/null 2>&1",
            dst_char[0], dst_char[1], dst_char[2], dst_char[3], prefix_len,
            nexthop_char[0], nexthop_char[1], nexthop_char[2], nexthop_char[3]);
    if (system(cmd)) {
        RETURN(BS_ERR);
    }

    return BS_OK;
}

static int _os_routecmd_Del(unsigned int dst, unsigned int prefix_len)
{
    char cmd[256];
    unsigned char *dst_char = (void*)&dst;

    snprintf(cmd, sizeof(cmd), "route -n delete -net %d.%d.%d.%d/%d > /dev/null 2>&1",
            dst_char[0], dst_char[1], dst_char[2], dst_char[3], prefix_len);
    if (system(cmd)) {
        RETURN(BS_ERR);
    }

    return BS_OK;
}
#endif

UINT Route_GetDefaultGw()
{
    return _os_route_GetDefaultGw();
}

int RouteCmd_AddStatic(unsigned int dst, unsigned int prefix_len, unsigned int nexthop)
{
    return _os_routecmd_Add(dst, prefix_len, nexthop, 1);
}

int RouteCmd_Add(unsigned int dst, unsigned int prefix_len, unsigned int nexthop)
{
    return _os_routecmd_Add(dst, prefix_len, nexthop, 0);
}

int RouteCmd_Del(unsigned int dst, unsigned int prefix_len)
{
    return _os_routecmd_Del(dst, prefix_len);
}
