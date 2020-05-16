/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-7-27
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
        
#include "utl/txt_utl.h"
#include "utl/file_utl.h"
#include "utl/url_lib.h"
#include "utl/lstr_utl.h"
#include "utl/ws_utl.h"
    
#include "ws_def.h"
#include "ws_context.h"
#include "ws_vhost.h"
#include "ws_trans.h"
#include "ws_deliver.h"

static _WS_DELIVER_NODE_S * ws_deliver_Find(IN DLL_HEAD_S *pstList, IN WS_DELIVER_TYPE_E enType, IN VOID *pKey)
{
    _WS_DELIVER_NODE_S *pstNode;
    _WS_DELIVER_NODE_S *pstNodeFound = NULL;
    UINT uiKeyLen = 0;

    if (enType != WS_DELIVER_TYPE_CALL_BACK)
    {
        uiKeyLen = strlen(pKey);
    }

    DLL_SCAN(pstList,pstNode)
    {
        if (enType != pstNode->enType)
        {
            continue;
        }

        if (enType == WS_DELIVER_TYPE_CALL_BACK)
        {
            if (pstNode->pKey == pKey)
            {
                pstNodeFound = pstNode;
                break;
            }
        }
        else
        {
            if ((uiKeyLen == pstNode->uiKeyLen) && (stricmp(pKey, pstNode->pKey) == 0))
            {
                pstNodeFound = pstNode;
                break;
            }
        }
    }

    return pstNodeFound;
}

static BOOL_T ws_deliver_MatchExact(IN LSTR_S *pstString, IN _WS_DELIVER_NODE_S *pstNode)
{
    if ((pstString->uiLen == pstNode->uiKeyLen)
        && (strnicmp(pstString->pcData, pstNode->pKey, pstNode->uiKeyLen) == 0))
    {
        return TRUE;
    }

    return FALSE;
}

static BOOL_T ws_deliver_MatchQuery(IN WS_TRANS_S *pstTrans, IN _WS_DELIVER_NODE_S *pstNode)
{
    MIME_HANDLE hMime;

    hMime = WS_Trans_GetQueryMime(pstTrans);
    if (NULL == hMime)
    {
        return FALSE;
    }

    if (NULL == MIME_GetKeyValue(hMime, pstNode->pKey))
    {
        return FALSE;
    }

    return TRUE;
}

static BOOL_T ws_deliver_MatchPrifix(IN LSTR_S *pstString, IN _WS_DELIVER_NODE_S *pstNode)
{
    if (pstString->uiLen == 0)
    {
        return FALSE;
    }

    if ((pstString->uiLen >= pstNode->uiKeyLen)
        && (strnicmp(pstString->pcData, pstNode->pKey, pstNode->uiKeyLen) == 0))
    {
        return TRUE;
    }

    return FALSE;
}

static inline BOOL_T ws_deliver_MatchNode
(
    IN _WS_DELIVER_NODE_S *pstNode,
    IN WS_TRANS_S *pstTrans,
    IN LSTR_S *pstMethod,
    IN LSTR_S *pstReferPath,
    IN LSTR_S *pstRequestFile,
    IN LSTR_S *pstExtName
)
{
    BOOL_T bMatched = FALSE;
    PF_WS_Deliver_MatchCB pfMatchCB;

    switch (pstNode->enType)
    {
        case WS_DELIVER_TYPE_METHOD:
        {
            bMatched = ws_deliver_MatchExact(pstMethod, pstNode);
            break;
        }

        case WS_DELIVER_TYPE_REFER_PATH:
        {
            bMatched = ws_deliver_MatchPrifix(pstReferPath, pstNode);
            break;
        }

        case WS_DELIVER_TYPE_FILE:
        {
            bMatched = ws_deliver_MatchExact(pstRequestFile, pstNode);
            break;
        }

        case WS_DELIVER_TYPE_PATH:
        {
            bMatched = ws_deliver_MatchPrifix(pstRequestFile, pstNode);
            break;
        }

        case WS_DELIVER_TYPE_EXT_NAME:
        {
            bMatched = ws_deliver_MatchExact(pstExtName, pstNode);
            break;
        }

        case WS_DELIVER_TYPE_QUERY:
        {
            bMatched = ws_deliver_MatchQuery(pstTrans, pstNode);
            break;
        }

        case WS_DELIVER_TYPE_CALL_BACK:
        {
            pfMatchCB = (PF_WS_Deliver_MatchCB) pstNode->pKey;
            BS_DBGASSERT(NULL != pfMatchCB);
            bMatched = pfMatchCB(pstTrans);
            break;
        }

        default:
        {
            BS_DBGASSERT(0);
            break;
        }
    }

    return bMatched;
}

static _WS_DELIVER_NODE_S * ws_deliver_Match
(
    IN DLL_HEAD_S *pstList,
    IN WS_TRANS_S *pstTrans,
    IN CHAR *pcMethod,
    IN CHAR *pcRefer,
    IN CHAR *pcRequestFile
)
{
    CHAR *pcExternName = NULL;
    LSTR_S stReferPath;
    LSTR_S stMethod;
    LSTR_S stRequestFile;
    LSTR_S stExtname;
    _WS_DELIVER_NODE_S *pstNode;
    _WS_DELIVER_NODE_S *pstNodeFound = NULL;

    stMethod.pcData = pcMethod;
    stMethod.uiLen = strlen(pcMethod);

    stRequestFile.pcData = pcRequestFile;
    stRequestFile.uiLen = strlen(pcRequestFile);

    if (NULL != pcRefer)
    {
        stReferPath.pcData = pcRefer;
        stReferPath.uiLen = strlen(pcRefer);
        URL_LIB_FullUrl2AbsPath(&stReferPath, &stReferPath);
    }
    else
    {
        LSTR_Init(&stReferPath);
    }

    pcExternName = FILE_GetExternNameFromPath(pcRequestFile, strlen(pcRequestFile));
    if (NULL != pcExternName)
    {
        stExtname.pcData = pcExternName;
        stExtname.uiLen = strlen(pcExternName);
    }
    else
    {
        LSTR_Init(&stExtname);
    }

    DLL_SCAN(pstList, pstNode)
    {
        if (TRUE == ws_deliver_MatchNode(pstNode, pstTrans, &stMethod, &stReferPath, &stRequestFile, &stExtname))
        {
            pstNodeFound = pstNode;
            break;
        }
    }

    return pstNodeFound;
}

static _WS_DELIVER_NODE_S * ws_deliver_AddNode
(
    IN DLL_HEAD_S *pstList,
    IN UINT uiPriority,
    IN WS_DELIVER_TYPE_E enType,
    IN VOID *pKey,
    IN PF_WS_Deliver_Func pfFunc,
    IN UINT uiFlag
)
{
    _WS_DELIVER_NODE_S *pstNode;
    _WS_DELIVER_NODE_S *pstNodeTmp = NULL;

    pstNode = MEM_ZMalloc(sizeof(_WS_DELIVER_NODE_S));
    if (NULL == pstNode)
    {
        return NULL;
    }

    pstNode->uiPriority = uiPriority;
    pstNode->enType = enType;
    pstNode->pKey = pKey;
    if (enType == WS_DELIVER_TYPE_CALL_BACK)
    {
        pstNode->uiKeyLen = 0;
    }
    else
    {
        pstNode->uiKeyLen = strlen(pKey);
    }
    pstNode->pfFunc = pfFunc;
    pstNode->uiFlag = uiFlag;

    DLL_SCAN(pstList, pstNodeTmp)
    {
        if (pstNodeTmp->uiPriority >= uiPriority)
        {
            DLL_INSERT_BEFORE(pstList, pstNode, pstNodeTmp);
            break;
        }
    }

    if (pstNodeTmp == NULL)
    {
        DLL_ADD(pstList, pstNode);
    }

    return pstNode;
}

static BS_STATUS ws_deliver_Add
(
    IN DLL_HEAD_S *pstList,
    IN UINT uiPriority,
    IN WS_DELIVER_TYPE_E enType,
    IN VOID *pKey,
    IN PF_WS_Deliver_Func pfFunc,
    IN UINT uiFlag
)
{
    if (NULL != ws_deliver_Find(pstList, enType, pKey))
    {
        return BS_ALREADY_EXIST;
    }

    if (NULL == ws_deliver_AddNode(pstList, uiPriority, enType, pKey, pfFunc, uiFlag))
    {
        return BS_NO_MEMORY;
    }

    return BS_OK;
}

BS_STATUS WS_Deliver_Reg
(
    IN WS_DELIVER_TBL_HANDLE hDeliverTbl,
    IN UINT uiPriority,
    IN WS_DELIVER_TYPE_E enType,
    IN VOID *pKey,
    IN PF_WS_Deliver_Func pfFunc,  /* 当为NULL时,表示匹配它的会被PASS,不进行Deliver */
    IN UINT uiFlag
)
{
    WS_DELIVER_TBL_S *pstTbl = hDeliverTbl;

    return ws_deliver_Add(&pstTbl->stDeliverList, uiPriority, enType, pKey, pfFunc, uiFlag); 
}

_WS_DELIVER_NODE_S * _WS_Deliver_Match(IN WS_TRANS_S *pstTrans)
{
    CHAR *pcMethod;
    CHAR *pcRefer;
    CHAR *pcRequestFile;
    _WS_DELIVER_NODE_S *pstNode = NULL;
    _WS_CONTEXT_S *pstContext = pstTrans->hContext;
    WS_DELIVER_TBL_S *pstTbl = pstContext->hDeliverTbl;

    if (pstTbl == NULL)
    {
        return NULL;
    }

    pcMethod = HTTP_GetMethodData(pstTrans->hHttpHeadRequest);
    BS_DBGASSERT(NULL != pcMethod);

    pcRefer = HTTP_GetHeadField(pstTrans->hHttpHeadRequest, HTTP_FIELD_REFERER);

    pcRequestFile = pstTrans->pcRequestFile;
    BS_DBGASSERT(NULL != pcRequestFile);

    pstNode = ws_deliver_Match(&pstTbl->stDeliverList, pstTrans, pcMethod, pcRefer, pcRequestFile);
    if ((NULL != pstNode) && (NULL == pstNode->pfFunc))
    {
        pstNode = NULL;
    }

    return pstNode;
}

WS_DELIVER_TBL_HANDLE WS_Deliver_Create()
{
    WS_DELIVER_TBL_S *pstTbl;

    pstTbl = MEM_ZMalloc(sizeof(WS_DELIVER_TBL_S));
    if (NULL == pstTbl)
    {
        return NULL;
    }

    DLL_INIT(&pstTbl->stDeliverList);

    return pstTbl;
}

