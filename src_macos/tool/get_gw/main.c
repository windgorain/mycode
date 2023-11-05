/*================================================================
*   Created by LiXingang：2018.11.06
*   Description：
*
================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <err.h>

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/route.h>


#ifndef SA_SIZE
# define SA_SIZE(sa)                        \
    (  (!(sa) || ((struct sockaddr *)(sa))->sa_len == 0) ?  \
           sizeof(long)     :               \
           1 + ( (((struct sockaddr *)(sa))->sa_len - 1) | (sizeof(long) - 1) ) )
#endif


static void ntreestuff(void)
{
    size_t needed;
    int mib[6];
    char *buf, *next, *lim;
    struct rt_msghdr *rtm;
    struct sockaddr *sa;
    struct sockaddr_in *sockin;
    char line[MAXHOSTNAMELEN];

    mib[0] = CTL_NET;
    mib[1] = PF_ROUTE;
    mib[2] = 0;
    mib[3] = 0;
    mib[4] = NET_RT_DUMP;
    mib[5] = 0;

    if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0) {
        err(1, "sysctl: net.route.0.0.dump estimate");
    }

    if ((buf = (char *)malloc(needed)) == NULL) {
        errx(2, "malloc(%lu)", (unsigned long)needed);
    }

    if (sysctl(mib, 6, buf, &needed, NULL, 0) < 0) {
        err(1, "sysctl: net.route.0.0.dump");
    }

    lim  = buf + needed;
    for (next = buf; next < lim; next += rtm->rtm_msglen) {
        rtm = (struct rt_msghdr *)next;
        sa = (struct sockaddr *)(rtm + 1);
        sa = (struct sockaddr *)(SA_SIZE(sa) + (char *)sa);
        sockin = (struct sockaddr_in *)sa;
        inet_ntop(AF_INET, &sockin->sin_addr.s_addr, line, sizeof(line) - 1);
        printf("defaultrouter=%s\n", line);
        break;
    }

    free(buf);
}

int main(int argc __unused, char *argv[] __unused)
{
    ntreestuff();
    return (0);
}
