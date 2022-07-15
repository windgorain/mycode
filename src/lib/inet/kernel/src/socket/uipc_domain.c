/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-11-15
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mbuf_utl.h"

#include "protosw.h"
#include "domain.h"


int max_hdr;

static DOMAIN_S * g_pstDomains = NULL;


VOID DOMAIN_Add(IN DOMAIN_S *pstDomain)
{
    pstDomain->pstDomNext = g_pstDomains;
    g_pstDomains = pstDomain;
}

PROTOSW_S * DOMAIN_FindProto(IN UINT uiFamily, IN USHORT usProtocol, IN USHORT usType)
{
    DOMAIN_S *dp;
    PROTOSW_S *pr;
    PROTOSW_S *maybe = NULL;
    
    if (uiFamily == 0)
    {
        return (0);
    }

    for (dp = g_pstDomains; dp != NULL; dp = dp->pstDomNext)
    {
        if (dp->uiDomFamily == uiFamily)
        {
            break;
        }
    }

    if (dp == NULL)
    {
        return (0);
    }

    for (pr = dp->pstProtoswStart; pr < dp->pstProtoswEnd; pr++)
    {
        if ((pr->pr_protocol == usProtocol) && (pr->pr_type == usType))
        {
            return (pr);
        }

        if ((usType == SOCK_RAW)
            && (pr->pr_type == SOCK_RAW)
            && (pr->pr_protocol == 0)
            && (maybe == NULL))
        {
            maybe = pr;
        }
    }

    return (maybe);
}

PROTOSW_S * DOMAIN_FindType(IN UINT uiFamily, IN USHORT usType)
{
    DOMAIN_S *dp;
    PROTOSW_S *pr;
    
    if (uiFamily == 0)
    {
        return (0);
    }

    for (dp = g_pstDomains; dp != NULL; dp = dp->pstDomNext)
    {
        if (dp->uiDomFamily == uiFamily)
        {
            break;
        }
    }

    if (dp == NULL)
    {
        return (0);
    }

    for (pr = dp->pstProtoswStart; pr < dp->pstProtoswEnd; pr++)
    {
        if ((pr->pr_type != 0) && (pr->pr_type == usType))
        {
            return pr;
        }
    }

    return NULL;
}


