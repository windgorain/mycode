/******************************************************************************
* Copyright (C),    LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2014-2-25
* Description: 
* History:     
******************************************************************************/

static DLL_HEAD_S g_stVsDomainList = DLL_HEAD_INIT_VALUE(&g_stVsDomainList);

VOID VS_DOMAIN_Add(IN VS_DOMAIN_S *pstDomain)
{
    DLL_ADD(&g_stVsDomainList, pstDomain);
}

static VS_DOMAIN_S * vs_domain_FindDomain(IN UINT uiFamily)
{
    VS_DOMAIN_S *pstDomain;

    if (uiFamily == 0)
    {
        return NULL;
    }

    DLL_SCAN(&g_stVsDomainList, pstDomain)
    {
        if (pstDomain->uiDomFamily == uiFamily)
        {
            return pstDomain;
        }
    }

    return NULL;
}

VS_PROTOSW_S * VS_DOMAIN_FindProto(IN UINT uiFamily, IN USHORT usProtocol, IN USHORT usType)
{
    VS_DOMAIN_S *pstDomain;
    VS_PROTOSW_S *pstProto;
    VS_PROTOSW_S *pstMaybe = NULL;
    UINT i;
    
    pstDomain = vs_domain_FindDomain(uiFamily);

    if (pstDomain == NULL)
    {
        return NULL;
    }

    for (i=0; i<pstDomain->uiProtoSwCount; i++)
    {
        pstProto = &pstDomain->pstProtoswArry[i];

        if ((pstProto->usProtocol == usProtocol) && (pstProto->usType == usType))
        {
            return pstProto;
        }

        if ((usType == SOCK_RAW)
            && (pstProto->usType == SOCK_RAW)
            && (pstProto->usProtocol == 0)
            && (pstMaybe == NULL))
        {
            pstMaybe = pstProto;
        }
    }

    return pstMaybe;
}

VS_PROTOSW_S * VS_DOMAIN_FindType(IN UINT uiFamily, IN USHORT usType)
{
    VS_DOMAIN_S *pstDomain;
    VS_PROTOSW_S *pstProto;
    VS_PROTOSW_S *pstMaybe = NULL;
    UINT i;
    
    pstDomain = vs_domain_FindDomain(uiFamily);

    if (pstDomain == NULL)
    {
        return NULL;
    }

    for (i=0; i<pstDomain->uiProtoSwCount; i++)
    {
        pstProto = &pstDomain->pstProtoswArry[i];

        if ((pstProto->usType != 0) && (pstProto->usType == usType))
        {
            return pstProto;
        }
    }

    return NULL;
}
