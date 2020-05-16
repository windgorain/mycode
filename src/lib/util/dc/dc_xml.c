/******************************************************************************
* Copyright (C), 2006-2016,  Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2012-05-03
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/cff_utl.h"
#include "utl/data2hex_utl.h"
#include "utl/dc_utl.h"
#include "utl/txt_utl.h"
#include "utl/xml_cfg.h"

#include "dc_proto_tbl.h"

#define DC_XML_HEAD_STRING          "XML"

#define DC_XML_TYPE_STRING          "_ISTBL"
#define DC_XML_TBL_KEY_STRING       "_KEY"
#define DC_XML_TBL_FIELD_STRING     "_FIELD"

static VOID dc_xml_BuildTableHead(OUT MKV_X_PARA_S *pstMkv, IN CHAR *pcTableName)
{
    pstMkv->apszMarkName[0] = DC_XML_HEAD_STRING;
    pstMkv->apszMarkName[1] = pcTableName;

    pstMkv->ulLevle = 2;
}

static BOOL_T dc_xml_IsOjbectKeyEquel(IN MKV_MARK_S *pstLi, IN DC_DATA_S *pstKey)
{
    UINT i;
    CHAR *pcValue;

    for (i=0; i<pstKey->uiNum; i++)
    {
        pcValue = XMLC_GetKeyValueInMark(pstLi, pstKey->astKeyValue[i].pcKey);
        if (NULL == pcValue)
        {
            return FALSE;
        }

        if (strcmp(pcValue, pstKey->astKeyValue[i].pcValue) != 0)
        {
            return FALSE;
        }
    }

    return TRUE;
}

static MKV_MARK_S * dc_xml_FindObject
(
    IN HANDLE hHandle,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey
)
{
    MKV_X_PARA_S stMkv;
    MKV_MARK_S *pstTableMark;
    MKV_MARK_S *pstLi;

    dc_xml_BuildTableHead(&stMkv, pcTableName);

    pstTableMark = XMLC_GetMark(hHandle, &stMkv);
    if (NULL == pstTableMark)
    {
        return NULL;
    }

    XMLC_SCAN_MARK_START(pstTableMark, pstLi)
    {
        if (TRUE == dc_xml_IsOjbectKeyEquel(pstLi, pstKey))
        {
            return pstLi;
        }
    }XMLC_SCAN_END();

    return NULL;
}

HANDLE DC_XML_OpenInstance(IN VOID *pParam)
{
    CHAR *pszIniFileName = pParam;

    return XMLC_Open(pszIniFileName, TRUE, FALSE, FALSE);
}

VOID DC_XML_CloseInstance(IN HANDLE hHandle)
{
    XMLC_Close(hHandle);
}

static MKV_MARK_S * dc_xml_GetObjectUl
(
    IN HANDLE hHandle,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey
)
{
    MKV_MARK_S *pstLi;
    MKV_MARK_S *pstUl;

    pstLi = dc_xml_FindObject(hHandle, pcTableName, pstKey);
    if (NULL == pstLi)
    {
        return NULL;
    }

    pstUl = XMLC_FindMarkInMark(hHandle, pstLi, "ul");
    if (NULL != pstUl)
    {
        return pstUl;
    }

    pstUl = XMLC_AddMark2Mark(hHandle, pstLi, "ul");

    return pstUl;
}

/* 设置一个对象的属性,如果对象不存在,则返回失败 */
BS_STATUS DC_XML_SetFieldValueAsUint
(
    IN HANDLE hHandle,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey,
    IN CHAR *pcFieldName,
    IN UINT uiValue
)
{
    MKV_MARK_S *pstUl;
    CHAR szValue[32];

    pstUl = dc_xml_GetObjectUl(hHandle, pcTableName, pstKey);
    if (NULL == pstUl)
    {
        return BS_NOT_FOUND;
    }

    sprintf(szValue, "%u", uiValue);

    return XMLC_SetKeyValueInMark(pstUl, pcFieldName, szValue);
}

/* 设置一个对象的属性,如果对象不存在,则返回失败 */
BS_STATUS DC_XML_SetFieldValueAsString
(
    IN HANDLE hHandle,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey,
    IN CHAR *pcFieldName,
    IN CHAR *pcValue
)
{
    MKV_MARK_S *pstUl;

    pstUl = dc_xml_GetObjectUl(hHandle, pcTableName, pstKey);
    if (NULL == pstUl)
    {
        return BS_NOT_FOUND;
    }

    return XMLC_SetKeyValueInMark(pstUl, pcFieldName, pcValue);
}

BS_STATUS DC_XML_GetFieldValueAsUint
(
    IN HANDLE hHandle,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey,
    IN CHAR *pcFieldName,
    OUT UINT *puiValue
)
{
    MKV_MARK_S *pstUl;
    CHAR *pcValue;

    pstUl = dc_xml_GetObjectUl(hHandle, pcTableName, pstKey);
    if (NULL == pstUl)
    {
        return BS_NOT_FOUND;
    }

    pcValue = XMLC_GetKeyValueInMark(pstUl, pcFieldName);
    if (NULL == pcValue)
    {
        return BS_NO_SUCH;
    }

    if (BS_OK != TXT_Atoui(pcValue, puiValue))
    {
        return BS_ERR;
    }

    return BS_OK;
}

BS_STATUS DC_XML_CpyFieldValueAsString
(
    IN HANDLE hHandle,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey,
    IN CHAR *pcFieldName,
    OUT CHAR *pcValue,
    IN UINT uiValueMaxSize
)
{
    MKV_MARK_S *pstUl;
    CHAR *pcValueFound;

    pstUl = dc_xml_GetObjectUl(hHandle, pcTableName, pstKey);
    if (NULL == pstUl)
    {
        return BS_NOT_FOUND;
    }

    pcValueFound = XMLC_GetKeyValueInMark(pstUl, pcFieldName);
    if (NULL == pcValueFound)
    {
        return BS_NO_SUCH;
    }

    TXT_Strlcpy(pcValue, pcValueFound, uiValueMaxSize);

    return BS_OK;
}

BS_STATUS DC_XML_AddTbl
(
    IN HANDLE hHandle,
    IN CHAR *pcTableName
)
{
    MKV_X_PARA_S stMkv;

    dc_xml_BuildTableHead(&stMkv, pcTableName);

    return XMLC_AddMark(hHandle, &stMkv);
}

VOID DC_XML_DelTbl
(
    IN HANDLE hHandle,
    IN CHAR *pcTableName
)
{
    MKV_X_PARA_S stMkv;

    dc_xml_BuildTableHead(&stMkv, pcTableName);

    XMLC_DelMark(hHandle, &stMkv);
}

BOOL_T DC_XML_IsObjectExist
(
    IN HANDLE hHandle,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey
)
{
    if (dc_xml_FindObject(hHandle, pcTableName, pstKey) != NULL)
    {
        return TRUE;
    }

    return FALSE;
}

BS_STATUS DC_XML_AddObject
(
    IN HANDLE hHandle,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey
)
{
    MKV_X_PARA_S stMkv;
    UINT i;
    BS_STATUS eRet = BS_OK;
    MKV_MARK_S *pstTableMark;
    MKV_MARK_S *pstLi;

    if (TRUE == DC_XML_IsObjectExist(hHandle, pcTableName, pstKey))
    {
        return BS_ALREADY_EXIST;
    }

    dc_xml_BuildTableHead(&stMkv, pcTableName);

    pstTableMark = XMLC_GetMark(hHandle, &stMkv);
    if (NULL == pstTableMark)
    {
        return BS_NOT_INIT;
    }

    pstLi = MKV_AddMark2Mark(pstTableMark, "li", FALSE);
    if (NULL == pstLi)
    {
        return BS_NO_MEMORY;
    }

    for (i=0; i<pstKey->uiNum; i++)
    {
        eRet |= XMLC_SetKeyValueInMark(pstLi, pstKey->astKeyValue[i].pcKey, pstKey->astKeyValue[i].pcValue);
    }

    if (eRet != BS_OK)
    {
        MKV_DelMarkInMark(pstTableMark, pstLi);
    }

    return eRet;
}

VOID DC_XML_DelObject
(
    IN HANDLE hHandle,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey
)
{
    MKV_X_PARA_S stMkv;
    MKV_MARK_S *pstLi;
    MKV_MARK_S *pstTableMark;

    dc_xml_BuildTableHead(&stMkv, pcTableName);

    pstTableMark = XMLC_GetMark(hHandle, &stMkv);
    if (NULL == pstTableMark)
    {
        return;
    }

    pstLi = dc_xml_FindObject(hHandle, pcTableName, pstKey);
    if (NULL != pstLi)
    {
        MKV_DelMarkInMark(pstTableMark, pstLi);
    }

    return;
}

static BS_WALK_RET_E dc_xml_WalkTableEach
(
    IN MKV_MARK_S *pstRoot,
    IN MKV_MARK_S *pstMark,
    IN HANDLE hUserHandle
)
{
    PF_DC_WALK_TBL_CB_FUNC pfFunc;
    USER_HANDLE_S *pstUserHandle = hUserHandle;

    pfFunc = pstUserHandle->ahUserHandle[0];

    return pfFunc(pstMark->pucMarkName, pstUserHandle->ahUserHandle[1]);
}

static BS_WALK_RET_E dc_xml_WalkTableObjectEach
(
    IN MKV_MARK_S *pstRoot,
    IN MKV_MARK_S *pstMark,
    IN HANDLE hUserHandle
)
{
    PF_DC_WALK_OBJECT_CB_FUNC pfFunc;
    USER_HANDLE_S *pstUserHandle = hUserHandle;
    CHAR *pcKeyName = NULL;
    DC_DATA_S stKey;

    stKey.uiNum = 0;

    pfFunc = pstUserHandle->ahUserHandle[0];

    while (BS_OK == XMLC_GetNextKeyInMark(pstMark, &pcKeyName))
    {
        stKey.astKeyValue[stKey.uiNum].pcKey = pcKeyName;
        stKey.astKeyValue[stKey.uiNum].pcValue = XMLC_GetKeyValueInMark(pstMark, pcKeyName);
        stKey.uiNum ++;
    }

    return pfFunc(&stKey, pstUserHandle->ahUserHandle[1]);
}


VOID DC_XML_WalkTable
(
    IN HANDLE hHandle,
    IN PF_DC_WALK_TBL_CB_FUNC pfFunc,
    IN HANDLE hUserHandle
)
{
    MKV_X_PARA_S stMkv;
    MKV_MARK_S *pstRootMark;
    USER_HANDLE_S stUserHandle;

    stMkv.apszMarkName[0] = DC_XML_HEAD_STRING;
    stMkv.ulLevle = 1;

    pstRootMark = XMLC_GetMark(hHandle, &stMkv);
    if (NULL == pstRootMark)
    {
        return;
    }

    stUserHandle.ahUserHandle[0] = pfFunc;
    stUserHandle.ahUserHandle[1] = hUserHandle;

    XMLC_WalkMarkInMark(pstRootMark, dc_xml_WalkTableEach, &stUserHandle);
}

VOID DC_XML_WalkTableObject
(
    IN HANDLE hHandle,
    IN CHAR *pcTableName,
    IN PF_DC_WALK_OBJECT_CB_FUNC pfFunc,
    IN HANDLE hUserHandle
)
{
    MKV_X_PARA_S stMkv;
    MKV_MARK_S *pstTblMark;
    USER_HANDLE_S stUserHandle;

    dc_xml_BuildTableHead(&stMkv, pcTableName);

    pstTblMark = XMLC_GetMark(hHandle, &stMkv);
    if (NULL == pstTblMark)
    {
        return;
    }

    stUserHandle.ahUserHandle[0] = pfFunc;
    stUserHandle.ahUserHandle[1] = hUserHandle;

    XMLC_WalkMarkInMark(pstTblMark, dc_xml_WalkTableObjectEach, &stUserHandle);
}


BS_STATUS DC_XML_Save(IN HANDLE hHandle)
{
    return XMLC_Save(hHandle);
}

HSTRING DC_XML_ToString(IN HANDLE hHandle)
{
    return XMLC_ToString(hHandle);
}

