/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-11-15
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mbuf_utl.h"
#include "utl/ip_utl.h"
#include "utl/udp_utl.h"

#include "protosw.h"
#include "domain.h"
#include "in.h"
#include "udp_func.h"
#include "ip_options.h"
#include "ip_fwd.h"
#include "udp_var.h"
#include "in_pcb.h"
#include "inet_ip.h"

extern DOMAIN_S g_stInetDomain;

static UCHAR g_aucInProto[IPPROTO_MAX];

static PROTOSW_S g_astInetSw[] =
{
{
    &g_stInetDomain,
    SOCK_DGRAM,
    IPPROTO_UDP,
    PR_ATOMIC|PR_ADDR,
    
    udp_input,
    NULL,
    udp_ctlinput,
    ip_ctloutput,

    udp_Init,
    udp_servinit,
    NULL,
    udp_slowtimo,
    NULL,
    
    &udp_usrreqs
},

{
    &g_stInetDomain,
    SOCK_RAW,
    0,
    PR_ATOMIC|PR_ADDR,
    
    NULL,
    NULL,
    NULL,
    NULL,

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    NULL
}
};

static DOMAIN_S g_stInetDomain =
{
    AF_INET,
    "internet",
    g_astInetSw,
    &g_astInetSw[sizeof(g_astInetSw)/sizeof(g_astInetSw[0])],
    NULL
};

VOID IN_Proto_Init()
{
    PROTOSW_S *pr;
    INT i;

    pr = DOMAIN_FindProto(AF_INET, IPPROTO_RAW, SOCK_RAW);
    if (pr == NULL)
    {
        return;
    }

    for (i = 0; i < IPPROTO_MAX; i++)
    {
        g_aucInProto[i] = (UCHAR)(pr - g_astInetSw);
    }

    for (pr = g_stInetDomain.pstProtoswStart; pr < g_stInetDomain.pstProtoswEnd; pr++)
    {
        if ((pr->pr_protocol != 0) && (pr->pr_protocol != IPPROTO_RAW))
        {
            if (pr->pr_protocol < IPPROTO_MAX)
            {
                g_aucInProto[pr->pr_protocol] = (UCHAR)(pr - g_astInetSw);
            }
        }
    }
}


VOID IN_Proto_Input(IN MBUF_S *pstMbuf, IN UCHAR ucProto, IN UINT uiPayloadOffset)
{
    pr_input_t pfFunc;

    pfFunc = g_astInetSw[g_aucInProto[ucProto]].pr_input;
    if (NULL != pfFunc)
    {
        pfFunc(pstMbuf, uiPayloadOffset);
    }
    else
    {
        MBUF_Free(pstMbuf);
    }

    return;
}


