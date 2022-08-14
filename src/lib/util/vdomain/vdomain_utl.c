/******************************************************************************
* Copyright (C),    LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2014-8-28
* Description: 树形domain.
* History:     
******************************************************************************/
#include "bs.h"
    
#include "utl/txt_utl.h"
#include "utl/bitmap1_utl.h"
#include "utl/passwd_utl.h"
#include "utl/vdomain_utl.h"

#define VDOMAIN_MAX_DOMAIN_ID_STRING_LEN 31

#define VDOMAIN_DOMAIN_TYPE_MAX_LEN 31

#define VDOMAIN_TBL_NAME_MAX_LEN  63
#define VDOMAIN_MAX_DOMAIN_NUM 0xffff

#define VDOMAIN_INNER_TBL_PREFIX_TITLE '_'

#define VDOMAIN_TBL_CONFIG_PREFIX    "_config"
#define VDOMAIN_TBL_SUBDOMAIN_PREFIX "_subdomain"
#define VDOMAIN_TBL_USER_PREFIX      "_user"

typedef struct
{
    BITMAP_S stBitmap;
    HANDLE hDc;
}VDOMAIN_CTRL_S;

static CHAR * vdomain_DomainType2String(IN VDOMAIN_TYPE_E eType)
{
    if (eType == VDOMAIN_TYPE_SUPER)
    {
        return "super";
    }

    return "normal";
}

static VDOMAIN_TYPE_E vdomain_DomainTypeString2Type(IN CHAR *pcString)
{
    if (strcmp(pcString, "super") == 0)
    {
        return VDOMAIN_TYPE_SUPER;
    }

    if (strcmp(pcString, "normal") == 0)
    {
        return VDOMAIN_TYPE_NORMAL;
    }

    return VDOMAIN_TYPE_MAX;
}

static BS_WALK_RET_E vdomain_WalkDomainEach(IN CHAR *pcTblName, IN HANDLE hUserHandle)
{
    VDOMAIN_CTRL_S *pstCtrl = hUserHandle;
    DC_DATA_S stKey;
    UINT uiDomindID;

    stKey.astKeyValue[0].pcKey = "key";
    stKey.astKeyValue[0].pcValue = "0";
    stKey.uiNum = 1;

    if (BS_OK == DC_GetFieldValueAsUint(pstCtrl->hDc, pcTblName, &stKey, "domain-id", &uiDomindID))
    {
        BITMAP_SET(&pstCtrl->stBitmap, uiDomindID);
    }

    return BS_WALK_CONTINUE;
}

static BS_STATUS vdomain_Restore(IN VDOMAIN_CTRL_S *pstCtrl)
{
    DC_WalkTable(pstCtrl->hDc, vdomain_WalkDomainEach, pstCtrl);

    return BS_OK;
}

static BS_STATUS vdomain_AddDomain
(
    IN VDOMAIN_CTRL_S *pstCtrl,
    IN UINT uiSuperDomainID,
    IN UINT uiDomainID,
    IN VDOMAIN_TYPE_E eType
)
{
    CHAR szConfigTbl[VDOMAIN_TBL_NAME_MAX_LEN + 1];
    CHAR szUserTbl[VDOMAIN_TBL_NAME_MAX_LEN + 1];
    CHAR szSubDomainTbl[VDOMAIN_TBL_NAME_MAX_LEN + 1];
    CHAR szDomainID[VDOMAIN_MAX_DOMAIN_ID_STRING_LEN + 1];
    DC_DATA_S stKey;
    BS_STATUS eRet = BS_OK;

    snprintf(szConfigTbl, sizeof(szConfigTbl), "%s-%d", VDOMAIN_TBL_CONFIG_PREFIX, uiDomainID);
    snprintf(szUserTbl, sizeof(szUserTbl), "%s-%d", VDOMAIN_TBL_USER_PREFIX, uiDomainID);
    snprintf(szSubDomainTbl, sizeof(szSubDomainTbl), "%s-%d", VDOMAIN_TBL_SUBDOMAIN_PREFIX, uiDomainID);
    snprintf(szDomainID, sizeof(szDomainID), "%d", uiDomainID);

    stKey.astKeyValue[0].pcKey = "key";
    stKey.astKeyValue[0].pcValue = "0";
    stKey.uiNum = 1;

    eRet |= DC_AddTbl(pstCtrl->hDc, szConfigTbl);
    eRet |= DC_AddObject(pstCtrl->hDc, szConfigTbl, &stKey);
    eRet |= DC_SetFieldValueAsString(pstCtrl->hDc, szConfigTbl, &stKey, "type", vdomain_DomainType2String(eType));
    eRet |= DC_SetFieldValueAsString(pstCtrl->hDc, szConfigTbl, &stKey, "domain-id", szDomainID);
    eRet |= DC_AddTbl(pstCtrl->hDc, szUserTbl);
    eRet |= DC_AddTbl(pstCtrl->hDc, szSubDomainTbl);

    if (eRet != BS_OK)
    {
        DC_DelTbl(pstCtrl->hDc, szConfigTbl);
        DC_DelTbl(pstCtrl->hDc, szUserTbl);
        DC_DelTbl(pstCtrl->hDc, szSubDomainTbl);
        return eRet;
    }

    if (uiSuperDomainID != 0)
    {
        snprintf(szSubDomainTbl, sizeof(szSubDomainTbl), "%s-%d", VDOMAIN_TBL_SUBDOMAIN_PREFIX, uiSuperDomainID);
        stKey.astKeyValue[0].pcKey = "domain-id";
        stKey.astKeyValue[0].pcValue = szDomainID;
        stKey.uiNum = 1;
        eRet = DC_AddObject(pstCtrl->hDc, szSubDomainTbl, &stKey);
        if (eRet != BS_OK)
        {
            DC_DelTbl(pstCtrl->hDc, szConfigTbl);
            DC_DelTbl(pstCtrl->hDc, szUserTbl);
            DC_DelTbl(pstCtrl->hDc, szSubDomainTbl);
            return eRet;
        }
    }

    DC_Save(pstCtrl->hDc);

    return BS_OK;
}

VDOMAIN_HANDLE VDOMAIN_CreateInstance(IN DC_TYPE_E eType, IN VOID *pParam)
{
    VDOMAIN_CTRL_S *pstCtrl;

    pstCtrl = MEM_ZMalloc(sizeof(VDOMAIN_CTRL_S));
    if (NULL == pstCtrl)
    {
        return NULL;
    }

    if (BS_OK != BITMAP_Create(&pstCtrl->stBitmap, VDOMAIN_MAX_DOMAIN_NUM))
    {
        MEM_Free(pstCtrl);
        return NULL;
    }

    pstCtrl->hDc = DC_OpenInstance(eType, pParam);
    if (NULL == pstCtrl->hDc)
    {
        BITMAP_Destory(&pstCtrl->stBitmap);
        MEM_Free(pstCtrl);
        return NULL;
    }

    vdomain_Restore(pstCtrl);

    return pstCtrl;
}

UINT VDOMAIN_AddDomain(IN VDOMAIN_HANDLE hVDomain, IN UINT uiSuperDomainID, IN VDOMAIN_TYPE_E eType)
{
    VDOMAIN_CTRL_S *pstCtrl = hVDomain;
    UINT uiDomainID;

    if (NULL == hVDomain)
    {
        return 0;
    }

    uiDomainID = BITMAP1_GetAUnsettedBitIndex(&pstCtrl->stBitmap);
    if (uiDomainID == 0)
    {
        return 0;
    }

    if (BS_OK != vdomain_AddDomain(pstCtrl, uiSuperDomainID, uiDomainID, eType))
    {
        return 0;
    }

    BITMAP_SET(&pstCtrl->stBitmap, uiDomainID);

    return uiDomainID;
}

UINT VDOMAIN_AddRootDomain(IN VDOMAIN_HANDLE hVDomain, IN VDOMAIN_TYPE_E eType)
{
    VDOMAIN_CTRL_S *pstCtrl = hVDomain;
    UINT uiDomainID = VDOMAIN_ROOT_DOMAIN_ID; /* root domain的ID 是1 */

    if (NULL == hVDomain)
    {
        return 0;
    }

    if (BITMAP1_ISSET(&pstCtrl->stBitmap, uiDomainID))
    {
        return uiDomainID;
    }
    
    return VDOMAIN_AddDomain(hVDomain, 0, eType);
}

BOOL_T VDOMAIN_IsDomainExist(IN VDOMAIN_HANDLE hVDomain, IN UINT uiDomainID)
{
    VDOMAIN_CTRL_S *pstCtrl = hVDomain;

    if (NULL == hVDomain)
    {
        return FALSE;
    }

    if (BITMAP1_ISSET(&pstCtrl->stBitmap, uiDomainID))
    {
        return TRUE;
    }
    
    return FALSE;
}


UINT VDOMAIN_GetNextDomain(IN VDOMAIN_HANDLE hVDomain, IN UINT uiCurrentDomainID)
{
    VDOMAIN_CTRL_S *pstCtrl = hVDomain;

    if (uiCurrentDomainID == 0) {
        return VDOMAIN_ROOT_DOMAIN_ID;
    }

    return BITMAP1_GetBusyFrom(&pstCtrl->stBitmap, uiCurrentDomainID + 1);
}

VDOMAIN_TYPE_E VDOMAIN_GetDomainType(IN VDOMAIN_HANDLE hVDomain, IN UINT uiDomainID)
{
    CHAR szConfigTbl[VDOMAIN_TBL_NAME_MAX_LEN + 1];
    CHAR szDomainType[VDOMAIN_DOMAIN_TYPE_MAX_LEN + 1];
    DC_DATA_S stKey;
    BS_STATUS eRet = BS_OK;
    VDOMAIN_CTRL_S *pstCtrl = hVDomain;

    snprintf(szConfigTbl, sizeof(szConfigTbl), "%s-%d", VDOMAIN_TBL_CONFIG_PREFIX, uiDomainID);

    stKey.astKeyValue[0].pcKey = "key";
    stKey.astKeyValue[0].pcValue = "0";
    stKey.uiNum = 1;

    eRet = DC_CpyFieldValueAsString(pstCtrl->hDc, szConfigTbl, &stKey, "type", szDomainType, sizeof(szDomainType));
    if (eRet != BS_OK)
    {
        return VDOMAIN_TYPE_MAX;
    }

    return vdomain_DomainTypeString2Type(szDomainType);
}

BS_STATUS VDOMAIN_SetDomainName
(
    IN VDOMAIN_HANDLE hVDomain,
    IN UINT uiDomainID,
    IN CHAR *pcDomainName
)
{
    VDOMAIN_CTRL_S *pstCtrl = hVDomain;
    CHAR szConfigTbl[VDOMAIN_TBL_NAME_MAX_LEN + 1];
    DC_DATA_S stKey;
    BS_STATUS eRet = BS_OK;

    if (NULL == hVDomain)
    {
        return BS_NULL_PARA;
    }

    if (strlen(pcDomainName) > VDOMAIN_MAX_DOMAIN_NAME_LEN)
    {
        return BS_OUT_OF_RANGE;
    }

    stKey.astKeyValue[0].pcKey = "key";
    stKey.astKeyValue[0].pcValue = "0";
    stKey.uiNum = 1;

    snprintf(szConfigTbl, sizeof(szConfigTbl), "%s-%d", VDOMAIN_TBL_CONFIG_PREFIX, uiDomainID);

    eRet = DC_SetFieldValueAsString(pstCtrl->hDc, szConfigTbl, &stKey, "DomainName", pcDomainName);

    DC_Save(pstCtrl->hDc);

    return eRet;
}

BS_STATUS VDOMAIN_GetDomainName
(
    IN VDOMAIN_HANDLE hVDomain,
    IN UINT uiDomainID,
    OUT CHAR *pcBuf,
    IN UINT uiBufSize
)
{
    VDOMAIN_CTRL_S *pstCtrl = hVDomain;
    CHAR szConfigTbl[VDOMAIN_TBL_NAME_MAX_LEN + 1];
    DC_DATA_S stKey;

    if (NULL == hVDomain)
    {
        return BS_NULL_PARA;
    }

    snprintf(szConfigTbl, sizeof(szConfigTbl), "%s-%d", VDOMAIN_TBL_CONFIG_PREFIX, uiDomainID);

    stKey.astKeyValue[0].pcKey = "key";
    stKey.astKeyValue[0].pcValue = "0";
    stKey.uiNum = 1;

    return DC_CpyFieldValueAsString(pstCtrl->hDc, szConfigTbl, &stKey, "DomainName", pcBuf, uiBufSize);
}


BS_STATUS VDOMAIN_SetAdmin
(
    IN VDOMAIN_HANDLE hVDomain,
    IN UINT uiDomainID,
    IN CHAR *pcUserName,
    IN CHAR *pcPassWord
)
{
    VDOMAIN_CTRL_S *pstCtrl = hVDomain;
    CHAR szConfigTbl[VDOMAIN_TBL_NAME_MAX_LEN + 1];
    DC_DATA_S stKey;
    BS_STATUS eRet = BS_OK;
    CHAR szPasswordCiper[PW_HEX_CIPHER_LEN(VDOMAIN_MAX_PASSWORD_LEN) + 1];

    if (NULL == hVDomain)
    {
        return BS_NULL_PARA;
    }

    if ((strlen(pcUserName) > VDOMAIN_MAX_USER_NAME_LEN)
        || (strlen(pcPassWord) > VDOMAIN_MAX_PASSWORD_LEN))
    {
        return BS_OUT_OF_RANGE;
    }

    stKey.astKeyValue[0].pcKey = "key";
    stKey.astKeyValue[0].pcValue = "0";
    stKey.uiNum = 1;

    PW_HexEncrypt(pcPassWord, szPasswordCiper, sizeof(szPasswordCiper));

    snprintf(szConfigTbl, sizeof(szConfigTbl), "%s-%d", VDOMAIN_TBL_CONFIG_PREFIX, uiDomainID);

    eRet = DC_SetFieldValueAsString(pstCtrl->hDc, szConfigTbl, &stKey, "admin", pcUserName);
    eRet |= DC_SetFieldValueAsString(pstCtrl->hDc, szConfigTbl, &stKey, "password", szPasswordCiper);

    DC_Save(pstCtrl->hDc);

    return eRet;
}

/* 判断用户名密码是否匹配 */
BOOL_T VDOMAIN_CheckAdmin
(
    IN VDOMAIN_HANDLE hVDomain,
    IN UINT uiDomainID,
    IN CHAR *pcUserName,
    IN CHAR *pcPassWord
)
{
    VDOMAIN_CTRL_S *pstCtrl = hVDomain;
    CHAR szConfigTbl[VDOMAIN_TBL_NAME_MAX_LEN + 1];
    DC_DATA_S stKey;
    BS_STATUS eRet = BS_OK;
    CHAR szPasswordCiper[PW_HEX_CIPHER_LEN(VDOMAIN_MAX_PASSWORD_LEN) + 1];
    CHAR szPasswordCiperSaved[PW_HEX_CIPHER_LEN(VDOMAIN_MAX_PASSWORD_LEN) + 1];
    CHAR szUserName[VDOMAIN_MAX_USER_NAME_LEN + 1];

    if (NULL == hVDomain)
    {
        return FALSE;
    }

    if ((strlen(pcUserName) > VDOMAIN_MAX_USER_NAME_LEN)
        || (strlen(pcPassWord) > VDOMAIN_MAX_PASSWORD_LEN))
    {
        return FALSE;
    }

    snprintf(szConfigTbl, sizeof(szConfigTbl), "%s-%d", VDOMAIN_TBL_CONFIG_PREFIX, uiDomainID);

    stKey.astKeyValue[0].pcKey = "key";
    stKey.astKeyValue[0].pcValue = "0";
    stKey.uiNum = 1;

    eRet = DC_CpyFieldValueAsString(pstCtrl->hDc, szConfigTbl,
        &stKey, "admin", szUserName, sizeof(szUserName));

    eRet |= DC_CpyFieldValueAsString(pstCtrl->hDc, szConfigTbl,
        &stKey, "password", szPasswordCiperSaved, sizeof(szPasswordCiperSaved));

    if (eRet != BS_OK)
    {
        return FALSE;
    }

    PW_HexEncrypt(pcPassWord, szPasswordCiper, sizeof(szPasswordCiper));

    if ((strcmp(szUserName, szUserName) != 0)
        || (strcmp(szPasswordCiper, szPasswordCiperSaved) != 0))
    {
        return FALSE;
    }

    return TRUE;
}

BS_STATUS VDOMAIN_AddUser
(
    IN VDOMAIN_HANDLE hVDomain,
    IN UINT uiDomainID,
    IN CHAR *pcUserName,
    IN CHAR *pcPassWord,
    IN UINT uiUserFlag
)
{
    VDOMAIN_CTRL_S *pstCtrl = hVDomain;
    CHAR szUserTbl[VDOMAIN_TBL_NAME_MAX_LEN + 1];
    DC_DATA_S stKey;
    BS_STATUS eRet = BS_OK;
    CHAR szPasswordCiper[PW_HEX_CIPHER_LEN(VDOMAIN_MAX_PASSWORD_LEN) + 1];

    if (NULL == hVDomain)
    {
        return BS_NULL_PARA;
    }

    if ((strlen(pcUserName) > VDOMAIN_MAX_USER_NAME_LEN)
        || (strlen(pcPassWord) > VDOMAIN_MAX_PASSWORD_LEN))
    {
        return BS_OUT_OF_RANGE;
    }

    PW_HexEncrypt(pcPassWord, szPasswordCiper, sizeof(szPasswordCiper));

    snprintf(szUserTbl, sizeof(szUserTbl), "%s-%d", VDOMAIN_TBL_USER_PREFIX, uiDomainID);

    stKey.astKeyValue[0].pcKey = "name";
    stKey.astKeyValue[0].pcValue = pcUserName;
    stKey.uiNum = 1;

    eRet = DC_AddObject(pstCtrl->hDc, szUserTbl, &stKey);
    if (eRet != BS_OK)
    {
        return BS_ERR;
    }
    eRet = DC_SetFieldValueAsString(pstCtrl->hDc, szUserTbl, &stKey, "password", szPasswordCiper);
    eRet |= DC_SetFieldValueAsUint(pstCtrl->hDc, szUserTbl, &stKey, "flag", uiUserFlag);
    if (eRet != BS_OK)
    {
        DC_DelObject(pstCtrl->hDc, szUserTbl, &stKey);
        return BS_ERR;
    }

    DC_Save(pstCtrl->hDc);

    return BS_OK;
}

/* 判断用户名密码是否匹配 */
BOOL_T VDOMAIN_CheckUser
(
    IN VDOMAIN_HANDLE hVDomain,
    IN UINT uiDomainID,
    IN CHAR *pcUserName,
    IN CHAR *pcPassWord
)
{
    VDOMAIN_CTRL_S *pstCtrl = hVDomain;
    CHAR szUserTbl[VDOMAIN_TBL_NAME_MAX_LEN + 1];
    DC_DATA_S stKey;
    BS_STATUS eRet = BS_OK;
    CHAR szPasswordCiper[PW_HEX_CIPHER_LEN(VDOMAIN_MAX_PASSWORD_LEN) + 1];
    CHAR szPasswordCiperSaved[PW_HEX_CIPHER_LEN(VDOMAIN_MAX_PASSWORD_LEN) + 1];

    if (NULL == hVDomain)
    {
        return FALSE;
    }

    if ((strlen(pcUserName) > VDOMAIN_MAX_USER_NAME_LEN)
        || (strlen(pcPassWord) > VDOMAIN_MAX_PASSWORD_LEN))
    {
        return FALSE;
    }

    snprintf(szUserTbl, sizeof(szUserTbl), "%s-%d", VDOMAIN_TBL_USER_PREFIX, uiDomainID);

    stKey.astKeyValue[0].pcKey = "name";
    stKey.astKeyValue[0].pcValue = pcUserName;
    stKey.uiNum = 1;

    eRet = DC_CpyFieldValueAsString(pstCtrl->hDc, szUserTbl,
        &stKey, "password", szPasswordCiperSaved, sizeof(szPasswordCiperSaved));
    if (eRet != BS_OK)
    {
        return FALSE;
    }

    PW_HexEncrypt(pcPassWord, szPasswordCiper, sizeof(szPasswordCiper));

    if (strcmp(szPasswordCiper, szPasswordCiperSaved) != 0)
    {
        return FALSE;
    }

    return TRUE;
}

UINT VDOMAIN_GetUserFlag
(
    IN VDOMAIN_HANDLE hVDomain,
    IN UINT uiDomainID,
    IN CHAR *pcUserName
)
{
    VDOMAIN_CTRL_S *pstCtrl = hVDomain;
    CHAR szUserTbl[VDOMAIN_TBL_NAME_MAX_LEN + 1];
    DC_DATA_S stKey;
    UINT uiFlag;
    BS_STATUS eRet = BS_OK;

    if (NULL == hVDomain)
    {
        return 0;
    }

    if (strlen(pcUserName) > VDOMAIN_MAX_USER_NAME_LEN)
    {
        return 0;
    }

    snprintf(szUserTbl, sizeof(szUserTbl), "%s-%d", VDOMAIN_TBL_USER_PREFIX, uiDomainID);

    stKey.astKeyValue[0].pcKey = "name";
    stKey.astKeyValue[0].pcValue = pcUserName;
    stKey.uiNum = 1;

    eRet = DC_GetFieldValueAsUint(pstCtrl->hDc, szUserTbl, &stKey, "flag", &uiFlag);
    if (eRet != BS_OK)
    {
        return 0;
    }

    return uiFlag;
}

BS_STATUS VDOMAIN_AddTbl
(
    IN VDOMAIN_HANDLE hVDomain,
    IN UINT uiDomainID,
    IN CHAR *pcSuffix
)
{
    VDOMAIN_CTRL_S *pstCtrl = hVDomain;
    CHAR szTableName[VDOMAIN_TBL_NAME_MAX_LEN + 1];

    if ((NULL == pcSuffix) || (NULL == hVDomain))
    {
        return BS_NULL_PARA;
    }

    if (pcSuffix[0] == VDOMAIN_INNER_TBL_PREFIX_TITLE)
    {
        return BS_NOT_SUPPORT;
    }

    snprintf(szTableName, sizeof(szTableName), "%d-%s", uiDomainID, pcSuffix);

    if (BS_OK != DC_AddTbl(pstCtrl->hDc, szTableName))
    {
        return BS_ERR;
    }

    DC_Save(pstCtrl->hDc);

    return BS_OK;
}



