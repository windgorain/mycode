/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-4-24
* Description: Key-Value解析器
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/mem_cap.h"
#include "utl/lstr_utl.h"
#include "utl/kv_utl.h"

typedef struct
{
    UINT uiFlag;
    void *memcap;
    PF_KV_DECODE_FUNC pfDecode;
    DLL_HEAD_S stKeyValueList;
}_KV_CTRL_S;

typedef struct
{
    DLL_NODE_S stLinkNode;
    RCU_NODE_S stRcu;
    KV_S stKv;
}_KV_NODE_S;

static VOID kv_FreeNode(IN _KV_CTRL_S *pstCtrl, IN _KV_NODE_S *pstNode)
{
    if (pstNode->stKv.pcKey != NULL)
    {
        MemCap_Free(pstCtrl->memcap, pstNode->stKv.pcKey);
    }

    if (pstNode->stKv.pcValue != NULL)
    {
        MemCap_Free(pstCtrl->memcap, pstNode->stKv.pcValue);
    }

    MemCap_Free(pstCtrl->memcap, pstNode);
}

static CHAR * kv_DftDecode(IN LSTR_S *pstValue, void *memcap)
{
    CHAR *pcValueTmp;

    pcValueTmp = MemCap_Malloc(memcap, pstValue->uiLen + 1);
    if (NULL == pcValueTmp) {
        return NULL;
    }

    if (pstValue->uiLen > 0) {
        memcpy(pcValueTmp, pstValue->pcData, pstValue->uiLen);
    }

    pcValueTmp[pstValue->uiLen] = '\0';

    return pcValueTmp;
}

static BS_STATUS kv_AddKeyValue(IN _KV_CTRL_S *pstCtrl, IN LSTR_S *pstKey, IN LSTR_S *pstValue)
{
    _KV_NODE_S *pstNode;
    _KV_NODE_S *pstNodeTmp = NULL;
    CHAR *pcKeyTmp;
    CHAR *pcValueTmp;

    pstNode = MemCap_ZMalloc(pstCtrl->memcap, sizeof(_KV_NODE_S));
    if (NULL == pstNode) {
        return BS_NO_MEMORY;
    }

    pcKeyTmp = MemCap_Malloc(pstCtrl->memcap, pstKey->uiLen + 1);
    if (NULL == pcKeyTmp) {
        MemCap_Free(pstCtrl->memcap, pstNode);
        return BS_NO_MEMORY;
    }
    memcpy(pcKeyTmp, pstKey->pcData, pstKey->uiLen);
    pcKeyTmp[pstKey->uiLen] = '\0';
    pstNode->stKv.pcKey = pcKeyTmp;

    pcValueTmp = pstCtrl->pfDecode(pstValue, pstCtrl->memcap);
    if (NULL == pcValueTmp)
    {
        MemCap_Free(pstCtrl->memcap, pcKeyTmp);
        MemCap_Free(pstCtrl->memcap, pstNode);
        return BS_NO_MEMORY;
    }

    pcValueTmp[pstValue->uiLen] = '\0';
    pstNode->stKv.pcValue = pcValueTmp;

    if (pstCtrl->uiFlag & KV_FLAG_SORT_KEY) {
        DLL_SCAN(&pstCtrl->stKeyValueList, pstNodeTmp)
        {
            if (strcmp(pstNodeTmp->stKv.pcKey, pstNode->stKv.pcKey) > 0)
            {
                break;
            }
        }
    }

    if (NULL == pstNodeTmp) {
        DLL_ADD(&pstCtrl->stKeyValueList, pstNode);
    } else {
        DLL_INSERT_BEFORE(&pstCtrl->stKeyValueList, pstNode, pstNodeTmp);
    }

    return BS_OK;
}

static int kv_CmpKey(IN _KV_CTRL_S *pstCtrl, IN CHAR *pcKey, IN LSTR_S *pstKey)
{
    if (pstCtrl->uiFlag & KV_FLAG_CASE_SENSITIVE)
    {
        return LSTR_StrCmp(pstKey, pcKey);
    }

    return LSTR_StrCaseCmp(pstKey, pcKey);
}

static _KV_NODE_S * kv_GetNode(IN _KV_CTRL_S *pstCtrl, IN LSTR_S *pstKey)
{
    _KV_NODE_S *pstNode;

    DLL_SCAN(&pstCtrl->stKeyValueList, pstNode)
    {
        if (kv_CmpKey(pstCtrl, pstNode->stKv.pcKey, pstKey) == 0)
        {
            return pstNode;
        }
    }

    return NULL;
}


static VOID kv_DelNode(IN _KV_CTRL_S *pstCtrl, IN _KV_NODE_S *pstNode)
{
    if (NULL == pstNode)
    {
        return;
    }

    DLL_DEL(&pstCtrl->stKeyValueList, pstNode);

    kv_FreeNode(pstCtrl, pstNode);
}

static BS_STATUS kv_SetKeyValue(IN _KV_CTRL_S *pstCtrl, IN LSTR_S *pstKey, IN LSTR_S *pstValue)
{
    _KV_NODE_S *pstNode;

    if (! (pstCtrl->uiFlag & KV_FLAG_PERMIT_MUL_KEY))
    {
        pstNode = kv_GetNode(pstCtrl, pstKey);
        if (NULL != pstNode)
        {
            kv_DelNode(pstCtrl, pstNode);
        }
    }

    return kv_AddKeyValue(pstCtrl, pstKey, pstValue);
}

KV_HANDLE KV_Create(KV_PARAM_S *p)
{
    static KV_PARAM_S default_param = {0};
    _KV_CTRL_S *pstCtrl;

    if (p == NULL) {
        p = &default_param;
    }

    pstCtrl = MemCap_ZMalloc(p->memcap, sizeof(_KV_CTRL_S));
    if (NULL == pstCtrl)
    {
        return NULL;
    }

    pstCtrl->memcap = p->memcap;
    pstCtrl->uiFlag = p->uiFlag;

    DLL_INIT(&pstCtrl->stKeyValueList);

    pstCtrl->pfDecode = kv_DftDecode;

    return pstCtrl;
}

VOID KV_Destory(IN KV_HANDLE hKvHandle)
{
    _KV_CTRL_S *pstCtrl = hKvHandle;

    if (NULL == pstCtrl) {
        return;
    }

    KV_Reset(hKvHandle);

    MemCap_Free(pstCtrl->memcap, pstCtrl);

    return;
}

void KV_Reset(KV_HANDLE hKvHandle)
{
    _KV_CTRL_S *pstCtrl;
    _KV_NODE_S *pstNode;

    pstCtrl = hKvHandle;

    if (NULL == pstCtrl) {
        return;
    }

    while ((pstNode = DLL_FIRST(&pstCtrl->stKeyValueList))) {
        DLL_DEL(&pstCtrl->stKeyValueList, pstNode);
        kv_FreeNode(pstCtrl, pstNode);
    }

    return;
}

/* 设置解码函数 */
VOID KV_SetDecode(IN KV_HANDLE hKvHandle, IN PF_KV_DECODE_FUNC pfDecode)
{
    _KV_CTRL_S *pstCtrl;
    
    pstCtrl = hKvHandle;

    if (NULL == pstCtrl)
    {
        BS_DBGASSERT(0);
        return;
    }

    if (NULL != pfDecode)
    {
        pstCtrl->pfDecode = pfDecode;
    }
    else
    {
        pstCtrl->pfDecode = kv_DftDecode;
    }
}

/* 只解析一个key=value的情况 */
BS_STATUS KV_ParseOne(IN KV_HANDLE hKvHandle, IN LSTR_S *pstLstr, IN CHAR cEquelChar)
{
    LSTR_S stKey;
    LSTR_S stValue;
    _KV_CTRL_S *pstCtrl;
    
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

    return kv_SetKeyValue(pstCtrl, &stKey, &stValue);
}

BS_STATUS KV_Parse(IN KV_HANDLE hKvHandle, IN LSTR_S *pstLstr, IN CHAR cSeparator, IN CHAR cEquelChar)
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
        KV_ParseOne(hKvHandle, &stKeyValue, cEquelChar);
    }

    return BS_OK;
}

BS_STATUS KV_Build
(
    IN KV_HANDLE hKvHandle,
    IN CHAR cSeparator,
    IN CHAR cEquelChar,
    IN UINT uiBufSize,
    OUT CHAR *pcBuf
)
{
    _KV_CTRL_S *pstCtrl;
    _KV_NODE_S *pstNode = NULL;
    BOOL_T bFirst = TRUE;
    UINT uiBufSizeTmp = uiBufSize;
    INT iStordLen;
    CHAR *pcBufTmp = pcBuf;
    
    pstCtrl = hKvHandle;

    DLL_SCAN(&pstCtrl->stKeyValueList, pstNode)
    {
        if (uiBufSizeTmp <= 1)
        {
            return BS_OUT_OF_RANGE;
        }

        if (bFirst == TRUE)
        {
            bFirst = FALSE;
            iStordLen = scnprintf(pcBufTmp, uiBufSizeTmp, "%s%c%s",
                pstNode->stKv.pcKey, cEquelChar, pstNode->stKv.pcValue);
        }
        else
        {
            iStordLen = scnprintf(pcBufTmp, uiBufSizeTmp, "%c%s%c%s",
                cSeparator, pstNode->stKv.pcKey, cEquelChar, pstNode->stKv.pcValue);
        }

        if (iStordLen < 0)
        {
            return BS_OUT_OF_RANGE;
        }

        uiBufSizeTmp -= iStordLen;
        pcBufTmp += iStordLen;
    }

    return BS_OK;
}

CHAR * KV_GetKeyValue(IN KV_HANDLE hKvHandle, IN CHAR *pcKey)
{
    _KV_CTRL_S *pstCtrl;
    _KV_NODE_S *pstNode;
    LSTR_S stKey;

    pstCtrl = hKvHandle;

    stKey.pcData = pcKey;
    stKey.uiLen = strlen(pcKey);

    pstNode = kv_GetNode(pstCtrl, &stKey);
    if (NULL == pstNode)
    {
        return NULL;
    }

    return pstNode->stKv.pcValue;
}

BS_STATUS KV_SetKeyValue(IN KV_HANDLE hKvHandle, IN CHAR *pcKey, IN CHAR *pcValue)
{
    _KV_CTRL_S *pstCtrl;
    LSTR_S stKey;
    LSTR_S stValue;
    _KV_NODE_S *pstNodeOld = NULL;
    BS_STATUS eRet;

    if ((NULL == hKvHandle) || (NULL == pcKey))
    {
        return BS_NULL_PARA;
    }

    pstCtrl = hKvHandle;

    stKey.pcData = pcKey;
    stKey.uiLen = strlen(pcKey);

    pstNodeOld = kv_GetNode(pstCtrl, &stKey);

    eRet = BS_OK;
    if (NULL != pcValue)
    {
        stValue.pcData = pcValue;
        stValue.uiLen = strlen(pcValue);

        eRet = kv_AddKeyValue(pstCtrl, &stKey, &stValue);
    }

    if (BS_OK == eRet)
    {
        kv_DelNode(pstCtrl, pstNodeOld);
    }

    return eRet;
}

KV_S * KV_GetNext(IN KV_HANDLE hKvHandle, IN KV_S *pstCurrent)
{
    _KV_CTRL_S *pstCtrl;
    _KV_NODE_S *pstNodeTmp = NULL;
    _KV_NODE_S * pstCurrNode;

    pstCtrl = hKvHandle;

    if ((NULL == pstCurrent) || (pstCurrent->pcKey == NULL) || (pstCurrent->pcKey[0] == '\0'))
    {
        pstNodeTmp = DLL_FIRST(&pstCtrl->stKeyValueList);
        if (NULL == pstNodeTmp)
        {
            return NULL;
        }
        return &pstNodeTmp->stKv;
    }

    pstCurrNode = container_of(pstCurrent, _KV_NODE_S, stKv);

    pstNodeTmp = DLL_NEXT(&pstCtrl->stKeyValueList, pstCurrNode);
    if (NULL != pstNodeTmp)
    {
        return &pstNodeTmp->stKv;
    }

    return NULL;
}

VOID KV_DelKey(IN KV_HANDLE hKvHandle, IN CHAR *pcKey)
{
    _KV_CTRL_S *pstCtrl;
    _KV_NODE_S *pstNode;
    LSTR_S stKey;

    pstCtrl = hKvHandle;

    stKey.pcData = pcKey;
    stKey.uiLen = strlen(pcKey);

    pstNode = kv_GetNode(pstCtrl, &stKey);
    if (NULL == pstNode)
    {
        return;
    }

    kv_DelNode(pstCtrl, pstNode);
}

VOID KV_Walk(IN KV_HANDLE hKvHandle, IN PF_KV_WALK_FUNC pfFunc, IN HANDLE hUserHandle)
{
    _KV_NODE_S *pstNode;
    _KV_CTRL_S *pstCtrl;

    if ((NULL == hKvHandle) || (NULL == pfFunc))
    {
        return;
    }

    pstCtrl = hKvHandle;

    DLL_SCAN(&pstCtrl->stKeyValueList, pstNode)
    {
        pfFunc(pstNode->stKv.pcKey, pstNode->stKv.pcValue, hUserHandle);
    }
}


