/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-6-8
* Description: URL匹配器, 根据Method, File, Path, 扩展名进行匹配
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/url_match.h"
#include "utl/file_utl.h"

typedef struct
{
    DLL_HEAD_S stMethodList;
    DLL_HEAD_S stFileList;
    DLL_HEAD_S stPathList;
    DLL_HEAD_S stExternNameList;
}_URL_MATCH_TBL_S;

/* 精确匹配 */
static URL_MATCH_NODE_S * url_match_Find(IN DLL_HEAD_S *pstList, IN CHAR *pcKey)
{
    URL_MATCH_NODE_S *pstNode;
    URL_MATCH_NODE_S *pstNodeFound = NULL;
    UINT uiKeyLen;

    uiKeyLen = strlen(pcKey);

    DLL_SCAN(pstList,pstNode)
    {
        if ((uiKeyLen == pstNode->uiKeyLen)
            && (stricmp(pcKey, pstNode->pcKey) == 0))
        {
            pstNodeFound = pstNode;
            break;
        }
    }

    return pstNodeFound;
}

/* 最长匹配 */
static URL_MATCH_NODE_S * url_match_MatchPath(IN DLL_HEAD_S *pstList, IN CHAR *pcUrl)
{
    URL_MATCH_NODE_S *pstNode;
    URL_MATCH_NODE_S *pstNodeFound = NULL;
    UINT uiUrlLen;
    UINT uiMatchLen = 0;

    uiUrlLen = strlen(pcUrl);

    DLL_SCAN(pstList,pstNode)
    {
        if ((uiUrlLen >= pstNode->uiKeyLen) && (strnicmp(pcUrl, pstNode->pcKey, pstNode->uiKeyLen) == 0))
        {
            if (pstNode->uiKeyLen > uiMatchLen)
            {
                pstNodeFound = pstNode;
                uiMatchLen = pstNode->uiKeyLen;
            }
        }
    }

    return pstNodeFound;
}

static URL_MATCH_NODE_S * url_match_MatchExternName(IN DLL_HEAD_S *pstList, IN CHAR *pcUrl)
{
    CHAR *pcExternName;

    pcExternName = FILE_GetExternNameFromPath(pcUrl, strlen(pcUrl));
    if (NULL == pcExternName)
    {
        return NULL;
    }

    return url_match_Find(pstList, pcExternName);
}

static URL_MATCH_NODE_S * url_match_AddNode
(
    IN DLL_HEAD_S *pstList,
    IN CHAR *pcKey,
    IN UINT uiFlag,
    IN USER_HANDLE_S *pstUserHandle
)
{
    URL_MATCH_NODE_S *pstNode;

    pstNode = MEM_ZMalloc(sizeof(URL_MATCH_NODE_S));
    if (NULL == pstNode)
    {
        return NULL;
    }

    pstNode->pcKey = pcKey;
    pstNode->uiKeyLen = strlen(pcKey);
    pstNode->stUserHandle = *pstUserHandle;
    pstNode->uiFlag = uiFlag;

    DLL_ADD(pstList, pstNode);

    return pstNode;
}

static BS_STATUS url_match_Add
(
    IN DLL_HEAD_S *pstList,
    IN CHAR *pcKey,
    IN UINT uiFlag,
    IN USER_HANDLE_S *pstUserHandle
)
{
    if (NULL != url_match_Find(pstList, pcKey))
    {
        return BS_ALREADY_EXIST;
    }

    if (NULL == url_match_AddNode(pstList, pcKey, uiFlag, pstUserHandle))
    {
        return BS_NO_MEMORY;
    }

    return BS_OK;
}

URL_MATCH_HANDLE URL_Match_Create()
{
    _URL_MATCH_TBL_S *pstUd;

    pstUd = MEM_ZMalloc(sizeof(_URL_MATCH_TBL_S));
    if (NULL == pstUd)
    {
        return NULL;
    }

    DLL_INIT(&pstUd->stMethodList);
    DLL_INIT(&pstUd->stFileList);
    DLL_INIT(&pstUd->stPathList);
    DLL_INIT(&pstUd->stExternNameList);

    return pstUd;
}

BS_STATUS URL_Match_RegMethod
(
    IN URL_MATCH_HANDLE hUD,
    IN CHAR *pcMethod,
    IN UINT uiFlag,
    IN USER_HANDLE_S *pstUserHandle
)
{
    _URL_MATCH_TBL_S *pstUD = hUD;

    return url_match_Add(&pstUD->stMethodList, pcMethod, uiFlag, pstUserHandle);
}

BS_STATUS URL_Match_RegFile
(
    IN URL_MATCH_HANDLE hUD,
    IN CHAR *pcFile,
    IN UINT uiFlag,
    IN USER_HANDLE_S *pstUserHandle
)
{
    _URL_MATCH_TBL_S *pstUD = hUD;

    return url_match_Add(&pstUD->stFileList, pcFile, uiFlag, pstUserHandle);
}

BS_STATUS URL_Match_RegPath
(
    IN URL_MATCH_HANDLE hUD,
    IN CHAR *pcPath,
    IN UINT uiFlag,
    IN USER_HANDLE_S *pstUserHandle
)
{
    _URL_MATCH_TBL_S *pstUD = hUD;

    return url_match_Add(&pstUD->stPathList, pcPath, uiFlag, pstUserHandle);
}

BS_STATUS URL_Match_RegExternName
(
    IN URL_MATCH_HANDLE hUD,
    IN CHAR *pcExternName,
    IN UINT uiFlag,
    IN USER_HANDLE_S *pstUserHandle
)
{
    _URL_MATCH_TBL_S *pstUD = hUD;

    return url_match_Add(&pstUD->stExternNameList, pcExternName, uiFlag, pstUserHandle);
}

/* 按照Method, File, Path, 扩展名顺序进行匹配 */
URL_MATCH_NODE_S * URL_Match_Match
(
    IN URL_MATCH_HANDLE hUD,
    IN CHAR *pcMethod,
    IN CHAR *pcRequestFile
)
{
    URL_MATCH_NODE_S *pstNode = NULL;
    _URL_MATCH_TBL_S *pstUD = hUD;

    pstNode = url_match_Find(&pstUD->stMethodList, pcMethod);
    if (NULL != pstNode)
    {
        return pstNode;
    }

    pstNode = url_match_Find(&pstUD->stFileList, pcRequestFile);
    if (NULL != pstNode)
    {
        return pstNode;
    }

    pstNode = url_match_MatchPath(&pstUD->stPathList, pcRequestFile);
    if (NULL != pstNode)
    {
        return pstNode;
    }

    pstNode = url_match_MatchExternName(&pstUD->stExternNameList, pcRequestFile);
    if (NULL != pstNode)
    {
        return pstNode;
    }

    return NULL;
}


