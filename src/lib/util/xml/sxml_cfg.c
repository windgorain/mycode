/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      lixingang  Version: 1.0  Date: 2007-2-8
* Description: simple xml cfg: 只有一级的XML Config文件
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/xml_cfg.h"

BS_STATUS SXMLC_Save(IN HANDLE hHandle)
{
    return XMLC_Save(hHandle);
}

VOID SXMLC_Close(IN HANDLE hHandle)
{
    XMLC_Close(hHandle);
}

HANDLE SXMLC_Open
(
    IN CHAR * pucFileName,
    IN BOOL_T bIsCreateIfNotExist,
    IN BOOL_T bSort,
    IN BOOL_T bReadOnly
)
{
    return XMLC_Open(pucFileName, bIsCreateIfNotExist, bSort, bReadOnly);
}

BS_STATUS SXMLC_DelKey(IN HANDLE hHandle, IN CHAR *pucMarkName, IN CHAR *pucKeyName)
{
    MKV_X_PARA_S stTreParam;

    stTreParam.apszMarkName[0] = pucMarkName;
    stTreParam.ulLevle = 1;
    
    return XMLC_DelKey(hHandle, &stTreParam, pucKeyName);
}

BS_STATUS SXMLC_DelAllKey(IN HANDLE hHandle, IN CHAR *pucMarkName)
{
    MKV_X_PARA_S stTreParam;

    stTreParam.apszMarkName[0] = pucMarkName;
    stTreParam.ulLevle = 1;

    return XMLC_DelAllKeyOfMark(hHandle, &stTreParam);
}

BS_STATUS SXMLC_DelSection(IN HANDLE hHandle, IN CHAR *pucMarkName)
{
    MKV_X_PARA_S stTreParam;

    stTreParam.apszMarkName[0] = pucMarkName;
    stTreParam.ulLevle = 1;

    return XMLC_DelMark(hHandle, &stTreParam);
}

BS_STATUS SXMLC_AddSection(IN HANDLE hHandle, IN CHAR *pucMarkName)
{
    MKV_X_PARA_S stTreParam;

    stTreParam.apszMarkName[0] = pucMarkName;
    stTreParam.ulLevle = 1;

    return XMLC_AddMark(hHandle, &stTreParam);
}

BS_STATUS SXMLC_GetKeyValueAsString(IN HANDLE hHandle, IN CHAR *pucMarkName, IN CHAR *pucKeyName, OUT CHAR **ppucKeyValue)
{
    MKV_X_PARA_S stTreParam;

    stTreParam.apszMarkName[0] = pucMarkName;
    stTreParam.ulLevle = 1;

    return XMLC_GetKeyValueAsString(hHandle, &stTreParam, pucKeyName, ppucKeyValue);
}

BOOL_T SXMLC_IsKeyExist(IN HANDLE hHandle, IN CHAR *pucMarkName, IN CHAR *pucKeyName)
{
    MKV_X_PARA_S stTreParam;

    stTreParam.apszMarkName[0] = pucMarkName;
    stTreParam.ulLevle = 1;

    return XMLC_IsKeyExist(hHandle, &stTreParam, pucKeyName);
}

CHAR * SXMLC_GetNextSec(IN HANDLE hHandle, IN CHAR *pcCruSecName)
{
    MKV_X_PARA_S stTreParam;

    stTreParam.ulLevle = 0;

    return XMLC_GetNextMark(hHandle, &stTreParam, pcCruSecName);
}

CHAR * SXMLC_GetSecByIndex(IN HANDLE hHandle, IN UINT uiIndex/* 从0开始计算 */)
{
    MKV_X_PARA_S stTreParam;

    stTreParam.ulLevle = 0;

    return XMLC_GetMarkByIndex(hHandle, &stTreParam, uiIndex);
}

BS_STATUS SXMLC_GetNextKey(IN HANDLE hHandle, IN CHAR *pucMarkName, INOUT CHAR **ppszKeyName)
{
    MKV_MARK_S *pstMarkRoot;
    MKV_X_PARA_S stTreParam;

    stTreParam.ulLevle = 0;

    pstMarkRoot = XMLC_GetMark(hHandle, &stTreParam);

    return XMLC_GetNextKeyInMark(pstMarkRoot, ppszKeyName);
}

BOOL_T SXMLC_IsSecExist(IN HANDLE hHandle, IN CHAR *pucMarkName)
{
    MKV_X_PARA_S stTreParam;

    stTreParam.apszMarkName[0] = pucMarkName;
    stTreParam.ulLevle = 1;

    return XMLC_IsMarkExist(hHandle, &stTreParam);
}

BS_STATUS SXMLC_SetKeyValueAsString(IN HANDLE hHandle, IN CHAR *pucMarkName, IN CHAR *pucKeyName, IN CHAR *pucKeyValue)
{
    MKV_X_PARA_S stTreParam;

    stTreParam.apszMarkName[0] = pucMarkName;
    stTreParam.ulLevle = 1;

    return XMLC_SetKeyValueAsString(hHandle, &stTreParam, pucKeyName, pucKeyValue);
}

BS_STATUS SXMLC_SetKeyValueAsUlong(IN HANDLE hHandle, IN CHAR *pucMarkName, IN CHAR *pucKeyName, IN UINT ulKeyValue)
{
    MKV_X_PARA_S stTreParam;

    stTreParam.apszMarkName[0] = pucMarkName;
    stTreParam.ulLevle = 1;

    return XMLC_SetKeyValueAsUlong(hHandle, &stTreParam, pucKeyName, ulKeyValue);
}

BS_STATUS SXMLC_GetKeyValueAsUint(IN HANDLE hHandle, IN CHAR *pucMarkName, IN CHAR *pucKeyName, OUT UINT *pulKeyValue)
{
    MKV_X_PARA_S stTreParam;

    stTreParam.apszMarkName[0] = pucMarkName;
    stTreParam.ulLevle = 1;

    return XMLC_GetKeyValueAsUint(hHandle, &stTreParam, pucKeyName, pulKeyValue);
}

BS_STATUS SXMLC_GetKeyValueAsInt(IN HANDLE hHandle, IN CHAR *pucMarkName, IN CHAR *pucKeyName, OUT INT *plKeyValue)
{
    MKV_X_PARA_S stTreParam;

    stTreParam.apszMarkName[0] = pucMarkName;
    stTreParam.ulLevle = 1;

    return XMLC_GetKeyValueAsInt(hHandle, &stTreParam, pucKeyName, plKeyValue);
}

/* 返回section的个数 */
UINT SXMLC_GetSectionNum(IN HANDLE hHandle)
{
    MKV_MARK_S *pstMarkRoot;
    MKV_X_PARA_S stTreParam;

    stTreParam.ulLevle = 0;

    pstMarkRoot = XMLC_GetMark(hHandle, &stTreParam);

    return XMLC_GetMarkNumInMark(pstMarkRoot);
}

static BS_WALK_RET_E _SXMLC_WalkTreSecFunc(IN MKV_MARK_S *pstMarkRoot, IN MKV_MARK_S *pstMark, IN USER_HANDLE_S *pstUserHandle)
{
    PF_SXMLC_SEC_WALK_FUNC pfFunc;
    
    pfFunc = (PF_SXMLC_SEC_WALK_FUNC) (pstUserHandle->ahUserHandle[1]);
    pfFunc(pstUserHandle->ahUserHandle[0], pstMark->pucMarkName, pstUserHandle->ahUserHandle[2]);

    return BS_WALK_CONTINUE;
}

VOID SXMLC_WalkSection(IN HANDLE hHandle, IN PF_SXMLC_SEC_WALK_FUNC pfFunc, IN HANDLE hUsrHandle)
{
    MKV_MARK_S *pstMarkRoot;
    MKV_X_PARA_S stTreParam;
    USER_HANDLE_S stUserHandle;

    stTreParam.ulLevle = 0;

    pstMarkRoot = XMLC_GetMark(hHandle, &stTreParam);

    if (NULL == pstMarkRoot)
    {
        return;
    }

    stUserHandle.ahUserHandle[0] = hHandle;
    stUserHandle.ahUserHandle[1] = pfFunc;
    stUserHandle.ahUserHandle[2] = hUsrHandle;

    XMLC_WalkMarkInMark(pstMarkRoot, (PF_MKV_MARK_WALK_FUNC)_SXMLC_WalkTreSecFunc, &stUserHandle);
}

static BS_WALK_RET_E _SXMLC_WalkTreKeyFunc(IN MKV_MARK_S *pstMarkRoot, IN MKV_KEY_S *pstKey, IN USER_HANDLE_S *pstUserHandle)
{
    PF_SXMLC_KEY_WALK_FUNC pfFunc;
    
    pfFunc = (PF_SXMLC_KEY_WALK_FUNC) (pstUserHandle->ahUserHandle[1]);
    return pfFunc(pstUserHandle->ahUserHandle[0], pstMarkRoot->pucMarkName, pstKey->pucKeyName, pstUserHandle->ahUserHandle[2]);
}

VOID SXMLC_WalkKey(IN HANDLE hHandle, IN CHAR *pszSecName, IN PF_SXMLC_KEY_WALK_FUNC pfFunc, IN HANDLE hUsrHandle)
{
    MKV_MARK_S *pstMarkRoot;
    MKV_X_PARA_S stTreParam;
    USER_HANDLE_S stUserHandle;

    stTreParam.apszMarkName[0] = pszSecName;
    stTreParam.ulLevle = 1;

    pstMarkRoot = XMLC_GetMark(hHandle, &stTreParam);

    if (NULL == pstMarkRoot)
    {
        return;
    }

    stUserHandle.ahUserHandle[0] = hHandle;
    stUserHandle.ahUserHandle[1] = pfFunc;
    stUserHandle.ahUserHandle[2] = hUsrHandle;

    XMLC_WalkKeyInMark(pstMarkRoot, (PF_MKV_KEY_WALK_FUNC)_SXMLC_WalkTreKeyFunc, &stUserHandle);
}

/* 返回section中属性的个数 */
UINT SXMLC_GetKeyNumOfSection(IN HANDLE hHandle, IN CHAR *pucMarkName)
{
    MKV_MARK_S *pstMarkRoot;
    MKV_X_PARA_S stTreParam;

    stTreParam.apszMarkName[0] = pucMarkName;
    stTreParam.ulLevle = 1;

    pstMarkRoot = XMLC_GetMark(hHandle, &stTreParam);

    return XMLC_GetKeyNumOfMark(pstMarkRoot);
}


