/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-7-16
* Description: 地址池配置的mf文件
* History:     
******************************************************************************/
#include "bs.h"
#include "utl/socket_utl.h"

#include "../h/svpn_def.h"
#include "../h/svpn_context.h"
#include "../h/svpn_ctxdata.h"
#include "../h/svpn_dweb.h"
#include "../h/svpn_mf.h"
#include "../h/svpn_ippool.h"

static BS_STATUS _svpn_ippoolmf_ModifyIpPool(IN MIME_HANDLE hMime,  IN SVPN_DWEB_S *pstDweb)
{
    CHAR szStartIP[24];
    CHAR szEndIP[24];
    CHAR *pcStartIP;
    CHAR *pcEndIP;
    UINT uiOldStartIP;
    UINT uiOldEndIP;
    UINT uiStartIP;
    UINT uiEndIP;
    CHAR *pcName;

    pcName = MIME_GetKeyValue(hMime, "Name");
    if ((NULL == pcName) || (pcName[0] == '\0'))
    {
        svpn_mf_SetFailed(pstDweb, "Empty name");
        return BS_ERR;
    }

    pcStartIP = MIME_GetKeyValue(hMime, "StartIP");
    pcEndIP = MIME_GetKeyValue(hMime, "EndIP");
    if ((NULL == pcStartIP) || (NULL == pcEndIP))
    {
        svpn_mf_SetFailed(pstDweb, "Bad param");
        return BS_ERR;
    }

    if (BS_OK != SVPN_CtxData_GetProp(pstDweb->hSvpnContext, SVPN_CTXDATA_IPPOOL, pcName, "StartIP", szStartIP, sizeof(szStartIP)))
    {
        svpn_mf_SetFailed(pstDweb, "Can't get startip");
        return BS_ERR;
    }

    if (BS_OK != SVPN_CtxData_GetProp(pstDweb->hSvpnContext, SVPN_CTXDATA_IPPOOL, pcName, "EndIP", szEndIP, sizeof(szEndIP)))
    {
        svpn_mf_SetFailed(pstDweb, "Can't get endip");
        return BS_ERR;
    }

    uiOldStartIP = Socket_NameToIpHost(szStartIP);
    uiOldEndIP = Socket_NameToIpHost(szEndIP);

    uiStartIP = Socket_NameToIpHost(pcStartIP);
    uiEndIP = Socket_NameToIpHost(pcEndIP);

    SVPN_IPPOOL_ModifyRange(pstDweb->hSvpnContext, uiOldStartIP, uiOldEndIP, uiStartIP, uiEndIP);

    return BS_OK;
}

static VOID _svpn_ippoolmf_DelRangeToIpPool(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcName)
{
    UINT uiStartIP;
    UINT uiEndIP;
    CHAR szStartIP[24];
    CHAR szEndIP[24];

    if (BS_OK != SVPN_CtxData_GetProp(hSvpnContext, SVPN_CTXDATA_IPPOOL, pcName, "StartIP", szStartIP, sizeof(szStartIP)))
    {
        return;
    }

    if (BS_OK != SVPN_CtxData_GetProp(hSvpnContext, SVPN_CTXDATA_IPPOOL, pcName, "EndIP", szEndIP, sizeof(szEndIP)))
    {
        return;
    }

    uiStartIP = Socket_NameToIpHost(szStartIP);
    uiEndIP = Socket_NameToIpHost(szEndIP);

    if ((uiStartIP == 0) || (uiEndIP == 0))
    {
        return;
    }

    SVPN_IPPOOL_DelRange(hSvpnContext, uiStartIP, uiEndIP);

    return;
}



static CHAR *g_apcSvpnIPPoolMfProperty[] = {"Description", "StartIP", "EndIP"};
static UINT g_uiSvpnIPPoolMfPropertyCount = sizeof(g_apcSvpnIPPoolMfProperty)/sizeof(CHAR*);
static VOID svpn_ippoolmf_IsExist(IN MIME_HANDLE hMime, IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonIsExist(hMime, pstDweb, SVPN_CTXDATA_IPPOOL);
}

static VOID svpn_ippoolmf_Add(IN MIME_HANDLE hMime,  IN SVPN_DWEB_S *pstDweb)
{
    BS_STATUS eRet;
    CHAR *pcStartIP;
    CHAR *pcEndIP;
    UINT uiStartIP;
    UINT uiEndIP;

    pcStartIP = MIME_GetKeyValue(hMime, "StartIP");
    pcEndIP = MIME_GetKeyValue(hMime, "EndIP");

    if ((NULL == pcStartIP) || (NULL == pcEndIP))
    {
        svpn_mf_SetFailed(pstDweb, "Bad param");
        return;
    }

    uiStartIP = Socket_NameToIpHost(pcStartIP);
    uiEndIP = Socket_NameToIpHost(pcEndIP);
    
    eRet = SVPN_IPPOOL_AddRange(pstDweb->hSvpnContext, uiStartIP, uiEndIP);
    if (eRet != BS_OK)
    {
        svpn_mf_SetFailed(pstDweb, "System error");
        return;
    }

    if (BS_OK != SVPN_MF_CommonAdd(hMime, pstDweb, SVPN_CTXDATA_IPPOOL, g_apcSvpnIPPoolMfProperty, g_uiSvpnIPPoolMfPropertyCount))
    {
        SVPN_IPPOOL_DelRange(pstDweb->hSvpnContext, uiStartIP, uiEndIP);
        return;
    }
}

static VOID svpn_ippoolmf_Del(IN MIME_HANDLE hMime,  IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonDelete(hMime, pstDweb, SVPN_CTXDATA_IPPOOL, _svpn_ippoolmf_DelRangeToIpPool);
}

static VOID svpn_ippoolmf_Modify(IN MIME_HANDLE hMime, IN SVPN_DWEB_S *pstDweb)
{
    if (BS_OK == _svpn_ippoolmf_ModifyIpPool(hMime, pstDweb))
    {
        SVPN_MF_CommonModify(hMime, pstDweb, SVPN_CTXDATA_IPPOOL, g_apcSvpnIPPoolMfProperty, g_uiSvpnIPPoolMfPropertyCount);
    }
}

static VOID svpn_ippoolmf_Get(IN MIME_HANDLE hMime, IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonGet(hMime, pstDweb, SVPN_CTXDATA_IPPOOL, g_apcSvpnIPPoolMfProperty, g_uiSvpnIPPoolMfPropertyCount);
}

static VOID svpn_ippoolmf_List(IN MIME_HANDLE hMime,  IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonList(hMime, pstDweb, SVPN_CTXDATA_IPPOOL, g_apcSvpnIPPoolMfProperty, g_uiSvpnIPPoolMfPropertyCount);
}


static SVPN_MF_MAP_S g_astSvpnIpPoolMfMap[] =
{
    {SVPN_USER_TYPE_ADMIN, "IPPool.IsExist",  svpn_ippoolmf_IsExist},
    {SVPN_USER_TYPE_ADMIN, "IPPool.Add",      svpn_ippoolmf_Add},
    {SVPN_USER_TYPE_ADMIN, "IPPool.Delete",   svpn_ippoolmf_Del},
    {SVPN_USER_TYPE_ADMIN, "IPPool.Modify",   svpn_ippoolmf_Modify},
    {SVPN_USER_TYPE_ADMIN, "IPPool.Get",      svpn_ippoolmf_Get},
    {SVPN_USER_TYPE_ADMIN|SVPN_USER_TYPE_USER, "IPPool.List",     svpn_ippoolmf_List},
};

BS_STATUS SVPN_IPPoolMf_Init()
{
    return SVPN_MF_Reg(g_astSvpnIpPoolMfMap, sizeof(g_astSvpnIpPoolMfMap)/sizeof(SVPN_MF_MAP_S));
}




PLUG_API BS_STATUS SVPN_IpPoolCmd_EnterView(int argc, char **argv, void *pEnv)
{
    return SVPN_CD_EnterView(pEnv, SVPN_CTXDATA_IPPOOL, argv[1]);
}


PLUG_API BS_STATUS SVPN_IpPoolCmd_SetDescription(int argc, char **argv,
        void *pEnv)
{
    return SVPN_CD_SetProp(pEnv, SVPN_CTXDATA_IPPOOL, "Description", argv[1]);
}


PLUG_API BS_STATUS SVPN_IpPoolCmd_SetStartAddress(int argc, char **argv,
        void *pEnv)
{
    return SVPN_CD_SetProp(pEnv, SVPN_CTXDATA_IPPOOL, "StartIP", argv[1]);
}


PLUG_API BS_STATUS SVPN_IpPoolCmd_SetEndAddress(int argc, char **argv,
        void *pEnv)
{
    return SVPN_CD_SetProp(pEnv, SVPN_CTXDATA_IPPOOL, "EndIP", argv[1]);
}

BS_STATUS SVPN_IpPoolCmd_Save(SVPN_CONTEXT_HANDLE hSvpnContext, HANDLE hFile)
{
    CHAR szName[SVPN_MAX_RES_NAME_LEN + 1] = "";
        
    while (BS_OK == SVPN_CtxData_GetNextObject(hSvpnContext, SVPN_CTXDATA_IPPOOL, szName, szName, SVPN_MAX_RES_NAME_LEN + 1))
    {
        if (0 != CMD_EXP_OutputMode(hFile, "ip-pool %s", szName)) {
            continue;
        }

        SVPN_CD_SaveProp(hSvpnContext, SVPN_CTXDATA_IPPOOL, szName, "Description", "description", hFile);
        SVPN_CD_SaveProp(hSvpnContext, SVPN_CTXDATA_IPPOOL, szName, "StartIP", "start-ip", hFile);
        SVPN_CD_SaveProp(hSvpnContext, SVPN_CTXDATA_IPPOOL, szName, "EndIP", "end-ip", hFile);

        CMD_EXP_OutputModeQuit(hFile);
    }

    return BS_OK;
}



