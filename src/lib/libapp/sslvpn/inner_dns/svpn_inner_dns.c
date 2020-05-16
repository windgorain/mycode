/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2017-4-26
* Description: 内网域名解析的相关配置
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/lstr_utl.h"

#include "../h/svpn_def.h"
#include "../h/svpn_context.h"
#include "../h/svpn_ctxdata.h"
#include "../h/svpn_dweb.h"
#include "../h/svpn_mf.h"

static VOID svpn_innerdnsmf_Modify(IN MIME_HANDLE hMime, IN SVPN_DWEB_S *pstDweb)
{
    CHAR *pcMainDNS;
    CHAR *pcSlaveDNS;

    pcMainDNS = MIME_GetKeyValue(hMime, "MainDNS");
    pcSlaveDNS = MIME_GetKeyValue(hMime, "SlaveDNS");
    if (NULL == pcMainDNS)
    {
        pcMainDNS = "";
    }
    if (NULL == pcSlaveDNS)
    {
        pcSlaveDNS = "";
    }

    SVPN_CtxData_SetProp(pstDweb->hSvpnContext, SVPN_CTXDATA_InnerDNS, "InnerDNS", "MainDNS", pcMainDNS);
    SVPN_CtxData_SetProp(pstDweb->hSvpnContext, SVPN_CTXDATA_InnerDNS, "InnerDNS", "SlaveDNS", pcSlaveDNS);

    svpn_mf_SetSuccess(pstDweb);

    return;
}

static VOID svpn_innerdnsmf_Get(IN MIME_HANDLE hMime, IN SVPN_DWEB_S *pstDweb)
{
    CHAR szDnsString[32];

    if (BS_OK != SVPN_CtxData_GetProp(pstDweb->hSvpnContext,
            SVPN_CTXDATA_InnerDNS, "InnerDNS", "MainDNS", szDnsString, sizeof(szDnsString)))
    {
        szDnsString[0] = '\0';
    }

    cJSON_AddStringToObject(pstDweb->pstJson, "MainDNS", szDnsString);

    if (BS_OK != SVPN_CtxData_GetProp(pstDweb->hSvpnContext,
            SVPN_CTXDATA_InnerDNS, "InnerDNS", "SlaveDNS", szDnsString, sizeof(szDnsString)))
    {
        szDnsString[0] = '\0';
    }

    cJSON_AddStringToObject(pstDweb->pstJson, "SlaveDNS", szDnsString);

    svpn_mf_SetSuccess(pstDweb);
}

static SVPN_MF_MAP_S g_astSvpnInnerDnsMfMap[] =
{
    {SVPN_USER_TYPE_ADMIN, "InnerDNS.Modify",     svpn_innerdnsmf_Modify},
    {SVPN_USER_TYPE_ADMIN|SVPN_USER_TYPE_USER, "InnerDNS.Get",        svpn_innerdnsmf_Get},
};

BS_STATUS SVPN_InnerDNS_Init()
{
    return SVPN_MF_Reg(g_astSvpnInnerDnsMfMap, sizeof(g_astSvpnInnerDnsMfMap)/sizeof(SVPN_MF_MAP_S));
}


