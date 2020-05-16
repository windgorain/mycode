/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-10-13
* Description: ptr len kv. 指针长度版本的kv, key和value不产生copy,只会记录key、value的offset和len
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/lstr_utl.h"
#include "utl/plkv_utl.h"

typedef struct
{
    UINT uiFlag;
    DLL_HEAD_S stKeyValueList;
}_PLKV_CTRL_S;

typedef struct
{
    DLL_NODE_S stLinkNode;
    LSTR_S stKey;
    LSTR_S stValue;
}_PLKV_NODE_S;

static int plkv_CmpKey(IN _PLKV_CTRL_S *pstCtrl, IN LSTR_S *pstKey1, IN LSTR_S *pstKey2)
{
    if (pstCtrl->uiFlag & KV_FLAG_CASE_SENSITIVE)
    {
        return LSTR_Cmp(pstKey1, pstKey2);
    }

    return LSTR_CaseCmp(pstKey1, pstKey2);
}

static _PLKV_NODE_S * plkv_GetNode(IN _PLKV_CTRL_S *pstCtrl, IN LSTR_S *pstKey)
{
    _PLKV_NODE_S *pstNode;

    DLL_SCAN(&pstCtrl->stKeyValueList, pstNode)
    {
        if (plkv_CmpKey(pstCtrl, &pstNode->stKey, pstKey) == 0)
        {
            return pstNode;
        }
    }

    return NULL;
}

static BS_STATUS plkv_AddKeyValue(IN _PLKV_CTRL_S *pstCtrl, IN LSTR_S *pstKey, IN LSTR_S *pstValue)
{
    _PLKV_NODE_S *pstNode;

    pstNode = MEM_ZMalloc(sizeof(_PLKV_NODE_S));
    if (NULL == pstNode)
    {
        return BS_NO_MEMORY;
    }

    pstNode->stKey = *pstKey;
    pstNode->stValue = *pstValue;

    DLL_ADD(&pstCtrl->stKeyValueList, pstNode);

    return BS_OK;
}

PLKV_HANDLE PLKV_Create(IN UINT uiFlag /* KV_FLAG_XXX */)
{
    _PLKV_CTRL_S *pstCtrl;

    pstCtrl = MEM_ZMalloc(sizeof(_PLKV_CTRL_S));
    if (NULL == pstCtrl)
    {
        return NULL;
    }

    DLL_INIT(&pstCtrl->stKeyValueList);
    pstCtrl->uiFlag = uiFlag;

    return pstCtrl;
}

VOID PLKV_Destroy(IN PLKV_HANDLE hKvHandle)
{
    _PLKV_CTRL_S *pstCtrl;
    _PLKV_NODE_S *pstNode, *pstNodeTmp;
    
    pstCtrl = hKvHandle;

    DLL_SAFE_SCAN(&pstCtrl->stKeyValueList, pstNode, pstNodeTmp)
    {
        DLL_DEL(&pstCtrl->stKeyValueList, pstNode);
        MEM_Free(pstNode);
    }
}

/* 只解析一个key=value的情况 */
BS_STATUS PLKV_ParseOne(IN PLKV_HANDLE hKvHandle, IN LSTR_S *pstLstr, IN CHAR cEquelChar)
{
    LSTR_S stKey;
    LSTR_S stValue;
    _PLKV_CTRL_S *pstCtrl;
    
    pstCtrl = hKvHandle;

    LSTR_Strim(pstLstr, TXT_BLANK_CHARS, pstLstr);

    if (pstLstr->uiLen == 0)
    {
        /* 空行返回 */
        return BS_OK;
    }

    if (pstLstr->pcData[0] == cEquelChar)
    {
        /* 第一个就是key和value之前的符号, 没有Key，返回 */
        return BS_BAD_PARA;
    }

    /* 分割字符串 */
    LSTR_Split(pstLstr, cEquelChar, &stKey, &stValue);

    return plkv_AddKeyValue(pstCtrl, &stKey, &stValue);
}

BS_STATUS PLKV_Parse(IN PLKV_HANDLE hKvHandle, IN LSTR_S *pstLstr, IN CHAR cSeparator, IN CHAR cEquelChar)
{
    LSTR_S stKeyValue;
    LSTR_S stLeft;
    
    if (pstLstr->uiLen == 0)
    {
        /* 空行返回 */
        return BS_OK;
    }

    stLeft = *pstLstr;

    while (stLeft.uiLen != 0)
    {
        LSTR_Split(&stLeft, cSeparator, &stKeyValue, &stLeft);
        PLKV_ParseOne(hKvHandle, &stKeyValue, cEquelChar);
    }

    return BS_OK;
}

LSTR_S * PLKV_GetKeyValue(IN PLKV_HANDLE hKvHandle, IN CHAR *pcKey)
{
    _PLKV_CTRL_S *pstCtrl;
    _PLKV_NODE_S *pstNode;
    LSTR_S stKey;

    pstCtrl = hKvHandle;

    stKey.pcData = pcKey;
    stKey.uiLen = strlen(pcKey);

    pstNode = plkv_GetNode(pstCtrl, &stKey);
    if (NULL == pstNode)
    {
        return NULL;
    }

    return &pstNode->stValue;
}


