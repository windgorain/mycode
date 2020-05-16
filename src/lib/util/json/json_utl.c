/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-11-1
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/lstr_utl.h"
#include "utl/json_utl.h"


VOID JSON_SetSuccess(IN cJSON * pstJson)
{
    cJSON_AddStringToObject(pstJson, "result", "Success");
}

VOID JSON_SetFailed(IN cJSON * pstJson, IN CHAR *pcReason)
{
    cJSON_AddStringToObject(pstJson, "result", "Failed");
    if (NULL != pcReason)
    {
       cJSON_AddStringToObject(pstJson, "reason", pcReason);
    }
}

BS_STATUS JSON_AppendInfo(IN cJSON * pstJson, IN CHAR *pcInfo)
{
    cJSON * pstInfo;
    cJSON *pstArray;

    pstArray = cJSON_GetObjectItem(pstJson, "infoarray");
    if (NULL == pstArray)
    {
        pstArray = cJSON_CreateArray();
        if (NULL == pstArray)
        {
            return BS_NO_MEMORY;
        }

        cJSON_AddItemToObject(pstJson, "infoarray", pstArray);
    }

    pstInfo = cJSON_CreateObject();
    if (NULL == pstInfo)
    {
        return BS_NO_MEMORY;
    }

    cJSON_AddStringToObject(pstInfo, "info", pcInfo);

    cJSON_AddItemToArray(pstArray, pstInfo);

    return BS_OK;
}

VOID JSON_AddString(IN cJSON * pstJson, IN CHAR *pcKey, IN CHAR *pcValue)
{
    cJSON_AddStringToObject(pstJson, pcKey, pcValue);
}

BS_STATUS JSON_NO_IsExist
(
    IN NO_HANDLE hNo,
    IN MIME_HANDLE hMime,
    IN cJSON * pstJson
)
{
    CHAR *pcName;

    pcName = MIME_GetKeyValue(hMime, "Name");
    if ((NULL == pcName) || (pcName[0] == '\0'))
    {
        JSON_SetFailed(pstJson, "Empty name");
        return BS_BAD_REQUEST;
    }

    if (NULL == NO_GetObjectByName(hNo, pcName))
    {
        cJSON_AddStringToObject(pstJson, "exist", "False");
    }
    else
    {
        cJSON_AddStringToObject(pstJson, "exist", "True");
    }

    JSON_SetSuccess(pstJson);

    return BS_OK;
}

BS_STATUS JSON_NO_List
(
    IN NO_HANDLE hNo,
    IN MIME_HANDLE hMime,
    IN cJSON * pstJson,
    IN CHAR **apcPropertys,
    IN UINT uiPropertyCount
)
{
    return JSON_NO_ListWithCallBack(hNo, hMime, pstJson, apcPropertys, uiPropertyCount, NULL, NULL);
}

BS_STATUS JSON_NO_ListWithCallBack
(
    IN NO_HANDLE hNo,
    IN MIME_HANDLE hMime,
    IN cJSON * pstJson,
    IN CHAR **apcPropertys,
    IN UINT uiPropertyCount,
    IN PF_JSON_LIST_IS_PERMIT pfIsPermit,
    IN HANDLE hUserHandle
)
{
    cJSON *pstArray;
    cJSON *pstResJson;
    UINT64 ulID = 0;
    UINT i;
    CHAR *pcProperty;

    pstArray = cJSON_CreateArray();
    if (NULL == pstArray)
    {
        JSON_SetFailed(pstJson, "Not enough memory");
        return BS_ERR;
    }

    while ((ulID = NO_GetNextID(hNo, ulID)) != 0)
    {
        if ((NULL != pfIsPermit) && (! pfIsPermit(hUserHandle, ulID)))
        {
            continue;
        }

        pstResJson = cJSON_CreateObject();
        if (NULL == pstResJson)
        {
            continue;
        }

        cJSON_AddStringToObject(pstResJson, "Name", NO_GetNameByID(hNo, ulID));

        for (i=0; i<uiPropertyCount; i++)
        {
            pcProperty = NO_GetKeyValueByID(hNo, ulID, apcPropertys[i]);
            if (NULL == pcProperty)
            {
                pcProperty = "";
            }
            cJSON_AddStringToObject(pstResJson, apcPropertys[i], pcProperty);
        }

        cJSON_AddItemToArray(pstArray, pstResJson);
    }

    cJSON_AddItemToObject(pstJson, "data", pstArray);

    JSON_SetSuccess(pstJson);

    return BS_OK;
}

/*
成功: BS_OK
已经存在: BS_ALREADY_EXIST
其他错误: ...
*/
BS_STATUS JSON_NO_Add
(
    IN NO_HANDLE hNo,
    IN MIME_HANDLE hMime,
    IN cJSON * pstJson,
    IN CHAR **apcPropertys,
    IN UINT uiPropertyCount
)
{
    CHAR *pcName;
    VOID *pNode;
    CHAR *pcPropertyValue;
    UINT i;

    pcName = MIME_GetKeyValue(hMime, "Name");
    if ((NULL == pcName) || (pcName[0] == '\0'))
    {
        JSON_SetFailed(pstJson, "Empty name");
        return BS_BAD_PARA;
    }

    if (NULL != NO_GetObjectByName(hNo, pcName))
    {
        JSON_SetFailed(pstJson, "Already exist");
        return BS_ALREADY_EXIST;
    }

    pNode = NO_NewObject(hNo, pcName);
    if (NULL == pNode)
    {
        JSON_SetFailed(pstJson, "Can't add");
        return BS_NO_MEMORY;
    }

    for (i=0; i<uiPropertyCount; i++)
    {
        pcPropertyValue = MIME_GetKeyValue(hMime, apcPropertys[i]);
        if (pcPropertyValue != NULL)
        {
            NO_SetKeyValue(hNo, pNode, apcPropertys[i], pcPropertyValue);
        }
    }

    JSON_SetSuccess(pstJson);

    return BS_OK;
}

BS_STATUS JSON_NO_Modify
(
    IN NO_HANDLE hNo,
    IN MIME_HANDLE hMime,
    IN cJSON * pstJson,
    IN CHAR **apcPropertys,
    IN UINT uiPropertyCount
)
{
    CHAR *pcName;
    VOID *pNode;
    CHAR *pcPropertyValue;
    UINT i;

    pcName = MIME_GetKeyValue(hMime, "Name");
    if ((NULL == pcName) || (pcName[0] == '\0'))
    {
        JSON_SetFailed(pstJson, "Empty name");
        return BS_ERR;
    }

    pNode = NO_GetObjectByName(hNo, pcName);
    if (NULL == pNode)
    {
        JSON_SetFailed(pstJson, "Not exist");
        return BS_NOT_FOUND;
    }

    for (i=0; i<uiPropertyCount; i++)
    {
        pcPropertyValue = MIME_GetKeyValue(hMime, apcPropertys[i]);
        if (pcPropertyValue != NULL)
        {
            NO_SetKeyValue(hNo, pNode, apcPropertys[i], pcPropertyValue);
        }
    }

    JSON_SetSuccess(pstJson);

    return BS_OK;
}

VOID JSON_NO_Get
(
    IN NO_HANDLE hNo,
    IN MIME_HANDLE hMime,
    IN cJSON * pstJson,
    IN CHAR **apcPropertys,
    IN UINT uiPropertyCount
)
{
    CHAR *pcName;
    VOID *pNode;
    CHAR *pcString;
    UINT i;

    pcName = MIME_GetKeyValue(hMime, "Name");
    if ((NULL == pcName) || (pcName[0] == '\0'))
    {
        JSON_SetFailed(pstJson, "Empty name");
        return;
    }

    pNode = NO_GetObjectByName(hNo, pcName);
    if (NULL == pNode)
    {
        JSON_SetFailed(pstJson, "Not exist");
        return;
    }

    cJSON_AddStringToObject(pstJson, "Name", pcName);

    for (i=0; i<uiPropertyCount; i++)
    {
        pcString = NO_GetKeyValue(pNode, apcPropertys[i]);
        if (NULL == pcString)
        {
            pcString = "";
        }

        cJSON_AddStringToObject(pstJson, apcPropertys[i], pcString);
    }

    JSON_SetSuccess(pstJson);

    return;
}

VOID JSON_NO_Delete
(
    IN NO_HANDLE hNo,
    IN MIME_HANDLE hMime,
    IN cJSON * pstJson
)
{
    JSON_NO_DeleteWithNotify(hNo, hMime, pstJson, NULL, NULL);
}

VOID JSON_NO_DeleteWithNotify
(
    IN NO_HANDLE hNo,
    IN MIME_HANDLE hMime,
    IN cJSON * pstJson,
    IN PF_JSON_DEL_NOTIFY pfDelNotify,
    IN HANDLE hUserHandle
)
{
    CHAR *pcNames;
    LSTR_S stName;
    CHAR szName[512];

    pcNames = MIME_GetKeyValue(hMime, "Delete");
    if (NULL == pcNames)
    {
        return;
    }

    LSTR_SCAN_ELEMENT_BEGIN(pcNames, strlen(pcNames), ',', &stName)
    {
        if ((stName.uiLen != 0) && (stName.uiLen < sizeof(szName)))
        {
            TXT_Strlcpy(szName, stName.pcData, stName.uiLen + 1);
            if ((NULL == pfDelNotify) || (FALSE != pfDelNotify(hUserHandle, szName)))
            {
                NO_FreeObjectByName(hNo, szName);
            }
        }
    }LSTR_SCAN_ELEMENT_END();

    JSON_SetSuccess(pstJson);

    return;
}

