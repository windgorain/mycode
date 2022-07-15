/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-11-26
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mbuf_utl.h"
#include "utl/ip_utl.h"
#include "utl/in_checksum.h"
#include "inet/in_pub.h"

#include "protosw.h"
#include "domain.h"
#include "in.h"
#include "udp_func.h"
#include "ip_options.h"
#include "uipc_func.h"
#include "uipc_compat.h"

#include "in_pcb.h"
#include "in_mcast.h"
#include "icmp.h"
#include "inet_ip.h"
#include "inet6_ip.h"
#include "ip_fwd.h"

#define OPTSET(bit) do {                        \
    INP_LOCK(inp);                          \
    if (0 != optval) {                        \
        inp->inp_flags |= bit;                  \
    }                                      \
    else {                                \
        inp->inp_flags &= ~bit;                 \
    }                                           \
    INP_UNLOCK(inp);                        \
} while (0)

#define OPTBIT(bit) (0 != (inp->inp_flags & bit) ? 1 : 0)


UCHAR inetctlerrmap[PRC_NCMDS] = 
{
    0,      0,      0,      0,
    0,      EMSGSIZE,   EHOSTDOWN,  EHOSTUNREACH,
    EHOSTUNREACH,   EHOSTUNREACH,   ECONNREFUSED,   ECONNREFUSED,
    EMSGSIZE,   EHOSTUNREACH,   0,      0,
    0,      0,      EHOSTUNREACH,   0,
    ENOPROTOOPT,    ECONNREFUSED
};

/*
接收到的UDP或RawIP报文，依据各种控制选项，从报文中提取相应字段，
       生成控制mbuf链，输出控制mbuf链即mp
*/
void ip_savecontrol
(
    IN INPCB_S *inp,
    OUT MBUF_S **mp,
    IN IP_HEAD_S *ip,
    IN MBUF_S *m,
    IN UCHAR *ip_header
)
{
    MBUF_S *mp0 = NULL;
    MBUF_S *mptmp = NULL;
    BS_STATUS ret;

    *mp = NULL;

    if ((inp->inp_flags & INP_RECVDSTADDR) != 0)
    {
        mptmp = sbcreatecontrol(&ip->unDstIp.uiIp, sizeof(UINT), IP_RECVDSTADDR, IPPROTO_IP);
        if (NULL != mptmp)
        {
            if (NULL != mp0)
            {
                ret = MBUF_Cat(mp0, mptmp);
                if (BS_OK != ret)
                {
                    (void)MBUF_Free(mptmp);
                }
            }
            else
            {
                mp0 = mptmp;
            }
        }
    }

    if ((inp->inp_flags & INP_RECVTTL) != 0)
    {
        mptmp = sbcreatecontrol(&ip->ucTtl, sizeof(UCHAR), IP_RECVTTL, IPPROTO_IP);
        if (NULL != mptmp)
        {
            if (NULL != mp0)
            {
                ret = MBUF_Cat(mp0, mptmp);
                if (BS_OK != ret)
                {
                    MBUF_Free(mptmp);
                }
            }
            else
            {
                mp0 = mptmp;
            }
        }
    }

    if ((inp->inp_flags & INP_RECVTOS) != 0)
    {
        mptmp = sbcreatecontrol(&ip->ucTos, sizeof(UCHAR), IP_RECVTOS, IPPROTO_IP);
        if (NULL != mptmp)
        {
            if (NULL != mp0)
            {
                ret = MBUF_Cat(mp0, mptmp);
                if (BS_OK != ret)
                {
                    MBUF_Free(mptmp);
                }
            }
            else
            {
                mp0 = mptmp;
            }
        }
    }

    if (0 != (inp->inp_flags & INP_RECVOPTS))
    {
        char *opt = (char *)(ip + 1);
        UINT optlen = IP_HEAD_LEN(ip) - sizeof(IP_HEAD_S);

        if (0 < optlen)
        {
            mptmp = sbcreatecontrol(opt, optlen, IP_RECVOPTS, IPPROTO_IP);
            if (NULL != mptmp)
            {
                if (NULL != mp0)
                {
                    ret = MBUF_Cat(mp0, mptmp);
                    if (BS_OK != ret)
                    {
                        MBUF_Free(mptmp);
                    }
                } else {
                    mp0 = mptmp;
                }
            }
        }
    }

    /*
     * 报文上送时可以携带报文的入接口索引，如果是从二层口上送的也携带入端口
     * 索引。
     */
    if ((inp->inp_flags & INP_RECVIF) != 0)
    {
        UINT uiRecvIfIndex = MBUF_GET_RECV_IF_INDEX(m);

        mptmp = sbcreatecontrol(&uiRecvIfIndex, sizeof(UINT), IP_RECVIF, IPPROTO_IP);
        if (NULL != mptmp)
        {
            if (NULL != mp0)
            {
                ret = MBUF_Cat(mp0, mptmp);
                if (BS_OK != ret)
                {
                    MBUF_Free(mptmp);
                }
            }
            else
            {
                mp0 = mptmp;
            }
        }
    }

    if ((inp->inp_flags & INP_RCVMACADDR) != 0)
    {
        IP_MAC_ADDR_S macaddr;

        memcpy(macaddr.im_dstmac, MBUF_GET_DESTMAC(m), 6);
        memcpy(macaddr.im_srcmac, MBUF_GET_SOURCEMAC(m), 6);

        mptmp = sbcreatecontrol(&macaddr, sizeof(macaddr), IP_RCVMACADDR, IPPROTO_IP);
        if (NULL != mptmp)
        {
            if (NULL != mp0)
            {
                ret = MBUF_Cat(mp0, mptmp);
                if (BS_OK != ret)
                {
                    (void)MBUF_Free(mptmp);
                }
            }
            else
            {
                mp0 = mptmp;
            }
        }
    }

    *mp = mp0;
    return;
}

/* 将应用程序设置的选项设置到mbuf中，此接口供TCP/UDP/RAWIP使用 */
void ip_setopt2mbuf
(
    IN INPCB_S *inp,
    IN MBUF_S *control,
    IN MBUF_S *m
)
{
    struct cmsghdr *cm;
    UINT contol_len;
    int opt;
    int hasdmac = 0;

    if (NULL != control) {
        while (MBUF_TOTAL_DATA_LEN(control) > 0)
        {
            /* First pullup, length is sizeof struct cmsghdr */
            if (BS_OK !=  MBUF_MakeContinue(control, sizeof(struct cmsghdr)))
            {
                break;
            }
            cm = MBUF_MTOD(control);
            contol_len = MBUF_TOTAL_DATA_LEN(control);
            if (contol_len < sizeof(*cm)
                || (cm->cmsg_len == 0)
                || (cm->cmsg_len > contol_len))
            {
                break;
            }

            if (cm->cmsg_level != IPPROTO_IP)
            {
                MBUF_CutHead(control, CMSG_ALIGN(cm->cmsg_len));
                continue;
            }

            if (BS_OK != MBUF_MakeContinue(control, CMSG_ALIGN(cm->cmsg_len)))
            {
                break;
            }
            cm = MBUF_MTOD(control);
            opt = cm->cmsg_type;
            switch (opt)
            {
                case IP_SNDBYDSTMAC:
                    /* 携带的结构为struct snd_mac */
                    if (cm->cmsg_len == CMSG_LEN(sizeof(struct snd_mac)))
                    {
                        MBUF_SET_DESTMAC(m, CMSG_DATA(cm));
                        MBUF_SET_ETH_MARKFLAG(m, MBUF_L2_FLAG_DST_MAC);
                        hasdmac = 1;
                    }
                    break;

                case IP_SNDBYSRCMAC:
                    /* 携带的结构为struct snd_mac */
                    if (cm->cmsg_len == CMSG_LEN(sizeof(struct snd_mac))) {
                        MBUF_SET_SOURCEMAC(m, (u_char *)CMSG_DATA(cm));
                        MBUF_SET_ETH_MARKFLAG(m, MBUF_L2_FLAG_SRC_MAC);
                    }
                    break;

                default:
                    break;
            }
            MBUF_CutHead(control, CMSG_ALIGN(cm->cmsg_len));
        }
    }

    if (NULL != inp)
    {
        INP_LOCK_ASSERT(inp);

        /*
         * 现在支持应用程序设置的选项有: 
         * 发送报文的目的MAC、
         * 协议报文发送、
         * 设置mplsflag
         */

        /* destnation MAC address */
        if ((0 == hasdmac) && (NULL != inp->inp_dmac))
        {
            /* copy destination MAC address to MBUF, and set Flag */
            MBUF_SET_DESTMAC(m, inp->inp_dmac->sm_mac);
            MBUF_SET_ETH_MARKFLAG(m, MBUF_L2_FLAG_DST_MAC);
        }
    }
    
    return;
}

int ip_pcbopts(IN INPCB_S *inp, IN int optname, IN MBUF_S *m)
{
    int cnt, optlen;
    UCHAR *cp;
    MBUF_S **pcbopt;
    UCHAR opt;
    (void) optname;

    INP_LOCK_ASSERT(inp);

    pcbopt = &inp->inp_options;

    /* turn off any old options */
    if (NULL != *pcbopt)
    {
        (void)MBUF_Free(*pcbopt);
    }

    *pcbopt = 0;
    cnt = (int)MBUF_TOTAL_DATA_LEN(m);
    if (m == NULL || cnt == 0)
    {
        /*
         * Only turning off any previous options.
         */
        if (m != NULL)
        {
            (void)MBUF_Free(m);
        }
        return (0);
    }

    if (0 != ((UINT)cnt % sizeof(UINT)))
    {
        goto bad;
    }

    /*
     * IP first-hop destination address will be stored before actual
     * options; move other options back and clear it when none present.
     */
    if (BS_OK != MBUF_Prepend(m, sizeof(UINT)))
    {
        goto bad;
    }

    if (BS_OK != MBUF_MakeContinue(m, sizeof(UINT) + cnt))
    {
        goto bad;
    }
    
    cp = MBUF_MTOD(m);
    Mem_Zero(cp, sizeof(UINT));
    cp += sizeof(UINT);

    for (; cnt > 0; cnt -= optlen, cp += optlen)
    {
        opt = cp[IPOPT_OPTVAL];
        if (opt == IPOPT_EOL)
        {
            break;
        }
        if (opt == IPOPT_NOP)
        {
            optlen = 1;
        }
        else
        {
            if (cnt < IPOPT_OLEN + (int)(UINT) sizeof(*cp)) {
                goto bad;
            }
            optlen = cp[IPOPT_OLEN];
            if (optlen < IPOPT_OLEN + (int)(UINT) sizeof(*cp) || optlen > cnt) {
                goto bad;
            }
        }
        switch (opt) {

            default:
                break;

            case IPOPT_LSRR:
            case IPOPT_SSRR:
                /*
                 * User process specifies route as:
                 *
                 *  ->A->B->C->D
                 *
                 * D must be our final destination (but we can't
                 * check that since we may not have connected yet).
                 * A is first hop destination, which doesn't appear
                 * in actual IP option, but is stored before the
                 * options.
                 */
                if (optlen < ((IPOPT_MINOFF - 1) + (int)(u_int) sizeof(struct in_addr))) {
                    goto bad;
                }
                cnt -= (int)(u_int) sizeof(struct in_addr);
                optlen -= (int)(u_int) sizeof(struct in_addr);
                cp[IPOPT_OLEN] = (u_char)(u_int) optlen;
                /*
                 * Move first hop before start of options.
                 */
                bcopy((caddr_t)&cp[IPOPT_OFFSET+1], MBUF_MTOD(m), sizeof(UINT));
                /*
                 * Then copy rest of options back
                 * to close up the deleted entry.
                 */
                bcopy((&cp[IPOPT_OFFSET+1] + sizeof(UINT)),
                        &cp[IPOPT_OFFSET+1],
                        ((unsigned)cnt - (IPOPT_MINOFF - 1)));
                MBUF_CutTail(m, sizeof(UINT));
                break;
        }
    }
    
    if (MBUF_TOTAL_DATA_LEN(m) > MAX_IPOPTLEN + sizeof(UINT))
    {
        goto bad;
    }
    *pcbopt = m;
    return (0);

bad:
    (void)MBUF_Free(m);
    return (EINVAL);
}

int ip_ctloutput(IN SOCKET_S *so, IN SOCKOPT_S *sopt)
{
    struct  inpcb *inp = sotoinpcb(so);
    int error, optval;
    struct snd_mac *macp;
    struct snd_mac mac;
    struct snd_if *outifp;
    struct snd_if outif;

    error = optval = 0;
    if (sopt->sopt_level != IPPROTO_IP)
    {
        return (EINVAL);
    }

    switch (sopt->sopt_dir)
    {
    case SOPT_SET:
        switch (sopt->sopt_name)
        {
        case IP_OPTIONS:
        case IP_RETOPTS:
        {
            MBUF_S *m;

            m = MBUF_CreateByCopyBuf(sizeof(UINT), sopt->sopt_val, sopt->sopt_valsize, MT_DATA);
            if (m == NULL)
            {
                error = ENOBUFS;
                break;
            }
            INP_LOCK(inp);
            error = ip_pcbopts(inp, sopt->sopt_name, m);
            INP_UNLOCK(inp);
            return (error);
        }

        case IP_SNDBYDSTMAC:
        {
            if (0 == sopt->sopt_valsize)
            {
                INP_LOCK(inp);
                macp = inp->inp_dmac;
                inp->inp_dmac = NULL;
                INP_UNLOCK(inp);
                if (NULL != macp)
                {
                    MEM_Free(macp);
                }
            }
            else
            {
                error = sooptcopyin(sopt, &mac, sizeof(struct snd_mac), sizeof(struct snd_mac));
                if (0 != error) {
                    break;
                }
                /* Copy to inpcb. */
                INP_LOCK(inp);
                if (NULL == inp->inp_dmac) {
                    macp = (struct snd_mac *)MEM_ZMalloc(sizeof(mac));
                    if (NULL == macp)
                    {
                        error = ENOMEM;
                        INP_UNLOCK(inp);
                        break;
                    }
                    inp->inp_dmac = macp;
                }
                memcpy(inp->inp_dmac, &mac, sizeof(struct snd_mac));
                INP_UNLOCK(inp);
            }
            break;
        }

        case IP_SNDDATAIF:
        {
            /* Option should be removed when option length equals 0. */
            if (0 == sopt->sopt_valsize)
            {
                INP_LOCK(inp);
                outifp = inp->inp_outif;
                inp->inp_outif = NULL;
                INP_UNLOCK(inp);
                if (NULL != outifp)
                {
                    MEM_Free(outifp);
                }
            }
            else
            {
                error = sooptcopyin(sopt, &outif, sizeof(struct snd_if), sizeof(struct snd_if));
                if (0 != error) {
                    break;
                }
                /* Copy to inpcb. */
                INP_LOCK(inp);
                if (NULL == inp->inp_outif) {
                    outifp = (struct snd_if *)MEM_ZMalloc(sizeof(outif));
                    if (NULL == outifp)
                    {
                        error = ENOMEM;
                        INP_UNLOCK(inp);
                        break;
                    }
                    inp->inp_outif = outifp;
                }
                memcpy(inp->inp_outif, &outif, sizeof(struct snd_if));
                INP_UNLOCK(inp);
            }
            break;
        }

        case IP_TOS:
        case IP_TTL:
        case IP_MINTTL:
        case IP_RECVOPTS:
        case IP_RECVRETOPTS:
        case IP_RECVDSTADDR:
        case IP_RECVTTL:
        case IP_RECVTOS:
        case IP_RECVIF:
        case IP_FAITH:
        case IP_ONESBCAST:
        case IP_DONTFRAG:
        case IP_RCVVLANID:
        case IP_RCVMACADDR:
        case IP_SNDBYLSPV:
            error = sooptcopyin(sopt, &optval, sizeof(optval),
                        sizeof(optval));
            if (0 != error)
            {
                break;
            }

            switch (sopt->sopt_name)
            {
            case IP_TOS:
                inp->inp_ip_tos = (UCHAR)(UINT) optval;
                break;

            case IP_TTL:
                inp->inp_ip_ttl = (UCHAR)(UINT) optval;
                break;

            case IP_MINTTL:
                if (optval > 0 && optval <= MAXTTL)
                {
                    inp->inp_ip_minttl = (UCHAR)(UINT) optval;
                }
                else {
                    error = EINVAL;
                }
                break;

            case IP_RECVOPTS:
                OPTSET(INP_RECVOPTS);
                break;

            case IP_RECVRETOPTS:
                OPTSET(INP_RECVRETOPTS);
                break;

            case IP_RECVDSTADDR:
                OPTSET(INP_RECVDSTADDR);
                break;

            case IP_RECVTTL:
                OPTSET(INP_RECVTTL);
                break;
                
            case IP_RECVTOS:
                OPTSET(INP_RECVTOS);
                break;
                
            case IP_RECVIF:
                OPTSET(INP_RECVIF);
                break;

            case IP_FAITH:
                OPTSET(INP_FAITH);
                break;

            case IP_ONESBCAST:
                OPTSET(INP_ONESBCAST);
                break;
            case IP_DONTFRAG:
                OPTSET(INP_DONTFRAG);
                break;
            case IP_RCVVLANID:
                OPTSET(INP_RCVVLANID);
                break;
            case IP_RCVMACADDR:
                OPTSET(INP_RCVMACADDR);
                break;
                
            case IP_SNDBYLSPV:
                OPTSET(INP_SNDBYLSPV);
                break;
                
            default:
                break;
            }
            break;
        /*
         * Multicast socket options are processed by the in_mcast
         * module.
         */
        case IP_MULTICAST_IF:
        case IP_MULTICAST_VIF:
        case IP_MULTICAST_TTL:
        case IP_MULTICAST_LOOP:
        case IP_ADD_MEMBERSHIP:
        case IP_DROP_MEMBERSHIP:
        case IP_ADD_SOURCE_MEMBERSHIP:
        case IP_DROP_SOURCE_MEMBERSHIP:
        case IP_BLOCK_SOURCE:
        case IP_UNBLOCK_SOURCE:
        case IP_MSFILTER:
        case MCAST_JOIN_GROUP:
        case MCAST_LEAVE_GROUP:
        case MCAST_JOIN_SOURCE_GROUP:
        case MCAST_LEAVE_SOURCE_GROUP:
        case MCAST_BLOCK_SOURCE:
        case MCAST_UNBLOCK_SOURCE:
            error = inp_setmoptions(inp, sopt);
            break;

        case IP_PORTRANGE:
            error = sooptcopyin(sopt, &optval, sizeof optval,
                        sizeof optval);
            if (0 != error) {
                break;
            }

            INP_LOCK(inp);
            switch (optval)
            {
            case IP_PORTRANGE_DEFAULT:
                inp->inp_flags &= ~(INP_LOWPORT);
                inp->inp_flags &= ~(INP_HIGHPORT);
                break;

            case IP_PORTRANGE_HIGH:
                inp->inp_flags &= ~(INP_LOWPORT);
                inp->inp_flags |= INP_HIGHPORT;
                break;

            case IP_PORTRANGE_LOW:
                inp->inp_flags &= ~(INP_HIGHPORT);
                inp->inp_flags |= INP_LOWPORT;
                break;

            default:
                error = EINVAL;
                break;
            }
            INP_UNLOCK(inp);
            break;

        default:
            error = ENOPROTOOPT;
            break;
        }
        break;

    case SOPT_GET:
        switch (sopt->sopt_name)
        {
        case IP_OPTIONS:
        case IP_RETOPTS:
            if (NULL != inp->inp_options)
            {
                error = sooptcopyout(sopt, 
                             MBUF_MTOD(inp->inp_options),
                             MBUF_TOTAL_DATA_LEN(inp->inp_options));
            }
            else
            {
                sopt->sopt_valsize = 0UL;
            }
            break;

        case IP_SNDBYDSTMAC:
        {
            if (NULL != inp->inp_dmac)
            {
                error = sooptcopyout(sopt, inp->inp_dmac, sizeof(struct snd_mac));
            }
            else
            {
                sopt->sopt_valsize = 0UL;
            }
            break;
        }

        case IP_SNDDATAIF:
        {
            if (NULL != inp->inp_outif)
            {
                error = sooptcopyout(sopt, inp->inp_outif, sizeof(struct snd_if));
            }
            else
            {
                sopt->sopt_valsize = 0UL;
            }
            break;
        }

        case IP_TOS:
        case IP_TTL:
        case IP_MINTTL:
        case IP_RECVOPTS:
        case IP_RECVRETOPTS:
        case IP_RECVDSTADDR:
        case IP_RECVTTL:
        case IP_RECVTOS:
        case IP_RECVIF:
        case IP_FAITH:
        case IP_ONESBCAST:
        case IP_DONTFRAG:
        case IP_RCVVLANID:
        case IP_RCVMACADDR:
        case IP_SNDBYLSPV:
            switch (sopt->sopt_name) {

            case IP_TOS:
                optval = inp->inp_ip_tos;
                break;

            case IP_TTL:
                optval = inp->inp_ip_ttl;
                break;

            case IP_MINTTL:
                optval = inp->inp_ip_minttl;
                break;

            case IP_RECVOPTS:
                optval = OPTBIT(INP_RECVOPTS);
                break;

            case IP_RECVRETOPTS:
                optval = OPTBIT(INP_RECVRETOPTS);
                break;

            case IP_RECVDSTADDR:
                optval = OPTBIT(INP_RECVDSTADDR);
                break;

            case IP_RECVTTL:
                optval = OPTBIT(INP_RECVTTL);
                break;

            case IP_RECVTOS:
                optval = OPTBIT(INP_RECVTOS);
                break;

            case IP_RECVIF:
                optval = OPTBIT(INP_RECVIF);
                break;
            /*
             * J03845: 在Leopard上端口通过主控板统一分配，不再使用BSD支持的算法
             * 也不再需要支持此选项
             *
            case IP_PORTRANGE:
                if (0 != (inp->inp_flags & INP_HIGHPORT)) {
                    optval = IP_PORTRANGE_HIGH;
                }
                else if (0 != (inp->inp_flags & INP_LOWPORT)) {
                    optval = IP_PORTRANGE_LOW;
                }
                else {
                    optval = 0;
                }
                break;
            */
            case IP_FAITH:
                optval = OPTBIT(INP_FAITH);
                break;

            case IP_ONESBCAST:
                optval = OPTBIT(INP_ONESBCAST);
                break;
                
            case IP_DONTFRAG:
                optval = OPTBIT(INP_DONTFRAG);
                break;

            case IP_RCVVLANID:
                optval = OPTBIT(INP_RCVVLANID);
                break;

            case IP_RCVMACADDR:
                optval = OPTBIT(INP_RCVMACADDR);
                break;
                
            case IP_SNDBYLSPV:
                optval = OPTBIT(INP_SNDBYLSPV);
                break;

            default:
                break;
            }
            error = sooptcopyout(sopt, &optval, sizeof(optval));
            break;
        /*
         * Multicast socket options are processed by the in_mcast
         * module.
         */
        case IP_MULTICAST_IF:
        case IP_MULTICAST_VIF:
        case IP_MULTICAST_TTL:
        case IP_MULTICAST_LOOP:
        case IP_MSFILTER:
            error = inp_getmoptions(inp, sopt);
            break;

        default:
            error = ENOPROTOOPT;
            break;
        }
        break;
    }

    return (error);
}



