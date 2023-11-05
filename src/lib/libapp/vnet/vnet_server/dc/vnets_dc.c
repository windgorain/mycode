/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-1-23
* Description: vnets 的数据容器
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/passwd_utl.h"
#include "utl/dc_utl.h"
#include "utl/txt_utl.h"
#include "utl/local_info.h"
#include "utl/ippool_utl.h"
#include "comp/comp_dc.h"

#include "../../inc/vnet_conf.h"
#include "../inc/vnets_dc.h"

#define _VNETS_DOMAIN_TBL_NAME "domain"
#define _VNETS_USER_TBL_NAME "user"

static DC_APP_HANDLE g_hVnetsDc;

BS_STATUS VNETS_DC_Init()
{
    g_hVnetsDc = DC_APP_Open(DC_TYPE_XML, "dc/xml/vnets_dc.xml");
    if (NULL == g_hVnetsDc)
    {
        BS_WARNNING(("Can't open file vnets_dc.xml."));
        return BS_ERR;
    }

    DC_APP_AddTbl(g_hVnetsDc, _VNETS_USER_TBL_NAME);
    DC_APP_AddTbl(g_hVnetsDc, _VNETS_DOMAIN_TBL_NAME);

#if 0  
    {
        DC_DATA_S stKey;
        CHAR szCipherText[PW_MD5_ENCRYPT_LEN + 1];

        PW_Md5Encrypt("111111", szCipherText);

        stKey.astKeyValue[0].pcKey = "user";
        stKey.astKeyValue[0].pcValue = "test";
        stKey.uiNum = 1;
        DC_AddObject(g_hVnetsDc, _VNETS_USER_TBL_NAME, &stKey);
        DC_SetFieldValueAsString(g_hVnetsDc, _VNETS_USER_TBL_NAME, &stKey, "password", szCipherText);

        stKey.astKeyValue[0].pcKey = "domain";
        stKey.astKeyValue[0].pcValue = "@test";
        stKey.uiNum = 1;
        DC_AddObject(g_hVnetsDc, _VNETS_DOMAIN_TBL_NAME, &stKey);
        DC_SetFieldValueAsUint(g_hVnetsDc, _VNETS_DOMAIN_TBL_NAME, &stKey, "broadcast", 0);

        DC_Save(g_hVnetsDc);
    }
#endif

    return BS_OK;
}

BOOL_T VNETS_DC_IsDomainExist(IN CHAR *pcDomainName)
{
    DC_DATA_S stKey;

    stKey.astKeyValue[0].pcKey = "domain";
    stKey.astKeyValue[0].pcValue = pcDomainName;
    stKey.uiNum = 1;

    return DC_APP_IsObjectExist(g_hVnetsDc, _VNETS_DOMAIN_TBL_NAME, &stKey);
}

BOOL_T VNETS_DC_IsBroadcastPermit(IN CHAR *pcDomainName)
{
    DC_DATA_S stKey;
    UINT uiValue;

    stKey.astKeyValue[0].pcKey = "domain";
    stKey.astKeyValue[0].pcValue = pcDomainName;
    stKey.uiNum = 1;

    if (BS_OK != DC_APP_GetFieldValueAsUint(g_hVnetsDc, _VNETS_DOMAIN_TBL_NAME, &stKey, "broadcast", &uiValue))
    {
        return FALSE;
    }

    if (0 == uiValue)
    {
        return FALSE;
    }

    return TRUE;
}


BOOL_T VNETS_DC_CheckUserPassword
(
    IN CHAR *pcUserName,
    IN CHAR *pcPassWord
)
{
    DC_DATA_S stKey;
    CHAR szTmp[128];

    stKey.astKeyValue[0].pcKey = "user";
    stKey.astKeyValue[0].pcValue = pcUserName;
    stKey.uiNum = 1;

    if (BS_OK != DC_APP_CpyFieldValueAsString(g_hVnetsDc, _VNETS_USER_TBL_NAME, &stKey, "password", szTmp, sizeof(szTmp)))
    {
        return FALSE;
    }

    if (TRUE != PW_Md5Check(pcPassWord, szTmp))
    {
        return FALSE;
    }

    return TRUE;
}

BS_STATUS VNETS_DC_AddUser(IN CHAR *pcUserName, IN CHAR *pcPassWord)
{
    DC_DATA_S stKey;
    CHAR szTmp[VNET_CONF_MAX_USER_NAME_LEN + 2];  

    stKey.astKeyValue[0].pcKey = "user";
    stKey.astKeyValue[0].pcValue = pcUserName;
    stKey.uiNum = 1;
    DC_APP_AddObject(g_hVnetsDc, _VNETS_USER_TBL_NAME, &stKey);
    VNETS_DC_SetPassword(pcUserName, pcPassWord);

    snprintf(szTmp, sizeof(szTmp), "@%s", pcUserName);

    stKey.astKeyValue[0].pcKey = "domain";
    stKey.astKeyValue[0].pcValue = szTmp;
    stKey.uiNum = 1;
    DC_APP_AddObject(g_hVnetsDc, _VNETS_DOMAIN_TBL_NAME, &stKey);
    DC_APP_SetFieldValueAsUint(g_hVnetsDc, _VNETS_DOMAIN_TBL_NAME, &stKey, "broadcast", 0);
    VNETS_DC_SetDhcpConfig(szTmp, "1", "10.41.41.1", "10.41.41.254", "255.255.255.0", "10.41.41.254");

    DC_APP_Save(g_hVnetsDc);

    return BS_OK;
}

BS_STATUS VNETS_DC_SetPassword(IN CHAR *pcUserName, IN CHAR *pcPassword)
{
    DC_DATA_S stKey;
    CHAR szCipherText[PW_MD5_ENCRYPT_LEN + 1];

    PW_Md5Encrypt(pcPassword, szCipherText);

    stKey.astKeyValue[0].pcKey = "user";
    stKey.astKeyValue[0].pcValue = pcUserName;
    stKey.uiNum = 1;
    DC_APP_SetFieldValueAsString(g_hVnetsDc, _VNETS_USER_TBL_NAME, &stKey, "password", szCipherText);

    DC_APP_Save(g_hVnetsDc);

    return BS_OK;
}

BS_STATUS VNETS_DC_SetEmail(IN CHAR *pcUserName, IN CHAR *pcEmail)
{
    DC_DATA_S stKey;

    stKey.astKeyValue[0].pcKey = "user";
    stKey.astKeyValue[0].pcValue = pcUserName;
    stKey.uiNum = 1;
    DC_APP_SetFieldValueAsString(g_hVnetsDc, _VNETS_USER_TBL_NAME, &stKey, "email", pcEmail);

    DC_APP_Save(g_hVnetsDc);

    return BS_OK;
}

BS_STATUS VNETS_DC_GetEmail(IN CHAR *pcUserName, OUT CHAR szEmail[VNET_DC_MAX_EMAIL_LEN + 1])
{
    DC_DATA_S stKey;

    stKey.astKeyValue[0].pcKey = "user";
    stKey.astKeyValue[0].pcValue = pcUserName;
    stKey.uiNum = 1;

    if (BS_OK != DC_APP_CpyFieldValueAsString(g_hVnetsDc, _VNETS_USER_TBL_NAME, &stKey, "email", szEmail, VNET_DC_MAX_EMAIL_LEN + 1))
    {
        return BS_NOT_FOUND;
    }

    return BS_OK;
}

BOOL_T VNETS_DC_IsUserExist(IN CHAR *pcUserName)
{
    DC_DATA_S stKey;

    stKey.astKeyValue[0].pcKey = "user";
    stKey.astKeyValue[0].pcValue = pcUserName;
    stKey.uiNum = 1;

    return DC_APP_IsObjectExist(g_hVnetsDc, _VNETS_USER_TBL_NAME, &stKey);
}

BS_STATUS VNETS_DC_SetDhcpConfig
(
    IN CHAR *pcDomain,
    IN CHAR *pcEnable,
    IN CHAR *pcStartIp,
    IN CHAR *pcEndIp,
    IN CHAR *pcMask,
    IN CHAR *pcGateway
)
{
    DC_DATA_S stKey;

    stKey.astKeyValue[0].pcKey = "domain";
    stKey.astKeyValue[0].pcValue = pcDomain;
    stKey.uiNum = 1;

    DC_APP_SetFieldValueAsString(g_hVnetsDc, _VNETS_DOMAIN_TBL_NAME, &stKey, "dhcp_enable", pcEnable);
    DC_APP_SetFieldValueAsString(g_hVnetsDc, _VNETS_DOMAIN_TBL_NAME, &stKey, "start_ip", pcStartIp);
    DC_APP_SetFieldValueAsString(g_hVnetsDc, _VNETS_DOMAIN_TBL_NAME, &stKey, "end_ip", pcEndIp);
    DC_APP_SetFieldValueAsString(g_hVnetsDc, _VNETS_DOMAIN_TBL_NAME, &stKey, "mask", pcMask);
    DC_APP_SetFieldValueAsString(g_hVnetsDc, _VNETS_DOMAIN_TBL_NAME, &stKey, "gateway", pcGateway);

    DC_APP_Save(g_hVnetsDc);

	return BS_OK;
}

BS_STATUS VNETS_DC_GetDhcpConfig
(
    IN CHAR *pcDomain,
    OUT CHAR *pcEnable,
    OUT CHAR *pcStartIp,
    OUT CHAR *pcEndIp,
    OUT CHAR *pcMask,
    OUT CHAR *pcGateway
)
{
    DC_DATA_S stKey;
    CHAR szTmp[32];

    stKey.astKeyValue[0].pcKey = "domain";
    stKey.astKeyValue[0].pcValue = pcDomain;
    stKey.uiNum = 1;

    szTmp[0] = '\0';
    DC_APP_CpyFieldValueAsString(g_hVnetsDc, _VNETS_DOMAIN_TBL_NAME, &stKey, "dhcp_enable", szTmp, sizeof(szTmp));
    TXT_StrCpy(pcEnable, szTmp);

    szTmp[0] = '\0';    
    DC_APP_CpyFieldValueAsString(g_hVnetsDc, _VNETS_DOMAIN_TBL_NAME, &stKey, "start_ip", szTmp, sizeof(szTmp));
    TXT_StrCpy(pcStartIp, szTmp);

    szTmp[0] = '\0';
    DC_APP_CpyFieldValueAsString(g_hVnetsDc, _VNETS_DOMAIN_TBL_NAME, &stKey, "end_ip", szTmp, sizeof(szTmp));
    TXT_StrCpy(pcEndIp, szTmp);

    szTmp[0] = '\0';
    DC_APP_CpyFieldValueAsString(g_hVnetsDc, _VNETS_DOMAIN_TBL_NAME, &stKey, "mask", szTmp, sizeof(szTmp));
    TXT_StrCpy(pcMask, szTmp);

    szTmp[0] = '\0';
    DC_APP_CpyFieldValueAsString(g_hVnetsDc, _VNETS_DOMAIN_TBL_NAME, &stKey, "gateway", szTmp, sizeof(szTmp));
    TXT_StrCpy(pcGateway, szTmp);

    return BS_OK;
}

