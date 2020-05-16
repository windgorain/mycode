/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-6-7
* Description: Key-Data, KV的更普适版本, 相比于KV,Value允许是一块内存，而不再限制为字符串了
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/lstr_utl.h"
#include "utl/kd_utl.h"

typedef struct
{
    DLL_HEAD_S stKeyDataList;  /* _KD_NODE_S */
}_KD_CTRL_S;

typedef struct
{
    DLL_NODE_S stLinkNode;
    KD_S stKd;
}_KD_NODE_S;

static VOID kd_FreeNode(IN DLL_NODE_S *pstDllNode)
{
    _KD_NODE_S *pstNode = (VOID*)pstDllNode;

    if (pstNode->stKd.pcKey != NULL)
    {
        MEM_Free(pstNode->stKd.pcKey);
    }

    if (pstNode->stKd.stData.uiLen != 0)
    {
        MEM_Free(pstNode->stKd.stData.pcData);
    }

    MEM_Free(pstNode);
}

static int kd_CmpKey(IN _KD_CTRL_S *pstCtrl, IN CHAR *pcKey, IN LSTR_S *pstKey)
{
    return LSTR_StrCaseCmp(pstKey, pcKey);
}

static _KD_NODE_S * kd_GetNode(IN _KD_CTRL_S *pstCtrl, IN LSTR_S *pstKey)
{
    _KD_NODE_S *pstNode;

    DLL_SCAN(&pstCtrl->stKeyDataList, pstNode)
    {
        if (kd_CmpKey(pstCtrl, pstNode->stKd.pcKey, pstKey) == 0)
        {
            return pstNode;
        }
    }

    return NULL;
}

static VOID kd_DelNode(IN _KD_CTRL_S *pstCtrl, IN _KD_NODE_S *pstNode)
{
    if (NULL == pstNode)
    {
        return;
    }

    DLL_DEL(&pstCtrl->stKeyDataList, pstNode);

    kd_FreeNode(&pstNode->stLinkNode);
}


static BS_STATUS kd_AddKeyValue(IN _KD_CTRL_S *pstCtrl, IN LSTR_S *pstKey, IN LSTR_S *pstData)
{
    _KD_NODE_S *pstNode;
    _KD_NODE_S *pstNodeTmp = NULL;
    CHAR *pcKeyTmp;
    UCHAR *pucValueTmp;

    pstNode = MEM_ZMalloc(sizeof(_KD_NODE_S));
    if (NULL == pstNode)
    {
        return BS_NO_MEMORY;
    }

    pcKeyTmp = MEM_Malloc(pstKey->uiLen + 1);
    if (NULL == pcKeyTmp)
    {
        MEM_Free(pstNode);
        return BS_NO_MEMORY;
    }

    memcpy(pcKeyTmp, pstKey->pcData, pstKey->uiLen);
    pcKeyTmp[pstKey->uiLen] = '\0';
    pstNode->stKd.pcKey = pcKeyTmp;

    if (pstData->uiLen > 0)
    {
        pucValueTmp = MEM_Malloc(pstData->uiLen);
        if (NULL == pucValueTmp)
        {
            MEM_Free(pcKeyTmp);
            MEM_Free(pstNode);
            return BS_NO_MEMORY;
        }
        memcpy(pucValueTmp, pstData->pcData, pstData->uiLen);
        pstNode->stKd.stData.pcData = (CHAR*)pucValueTmp;
        pstNode->stKd.stData.uiLen = pstData->uiLen;
    }
    else
    {
        pstNode->stKd.stData.pcData = pstData->pcData;
    }

    DLL_SCAN(&pstCtrl->stKeyDataList, pstNodeTmp)
    {
        if (strcmp(pstNodeTmp->stKd.pcKey, pstNode->stKd.pcKey) >= 0)
        {
            break;
        }
    }

    if (NULL == pstNodeTmp)
    {
        DLL_ADD(&pstCtrl->stKeyDataList, pstNode);
    }
    else
    {
        DLL_INSERT_BEFORE(&pstCtrl->stKeyDataList, pstNode, pstNodeTmp);
    }

    return BS_OK;
}


KD_HANDLE KD_Create()
{
    _KD_CTRL_S *pstCtrl;

    pstCtrl = MEM_ZMalloc(sizeof(_KD_CTRL_S));
    if (NULL == pstCtrl)
    {
        return NULL;
    }

    DLL_INIT(&pstCtrl->stKeyDataList);

    return pstCtrl;
}

VOID KD_Destory(IN KD_HANDLE hKDHandle)
{
    _KD_CTRL_S *pstCtrl;
    
    pstCtrl = hKDHandle;

    if (NULL == pstCtrl)
    {
        return;
    }

    DLL_FREE(&pstCtrl->stKeyDataList, kd_FreeNode);

    MEM_Free(pstCtrl);

    return;
}

LSTR_S * KD_GetKeyData(IN KD_HANDLE hKDHandle, IN CHAR *pcKey)
{
    _KD_CTRL_S *pstCtrl;
    _KD_NODE_S *pstNode;
    LSTR_S stKey;

    pstCtrl = hKDHandle;

    stKey.pcData = pcKey;
    stKey.uiLen = strlen(pcKey);

    pstNode = kd_GetNode(pstCtrl, &stKey);
    if (NULL == pstNode)
    {
        return NULL;
    }

    return &pstNode->stKd.stData;
}

BS_STATUS KD_SetKeyData(IN KD_HANDLE hKDHandle, IN CHAR *pcKey, IN LSTR_S *pstData)
{
    _KD_CTRL_S *pstCtrl;
    LSTR_S stKey;
    _KD_NODE_S *pstNodeOld;
    BS_STATUS eRet;

    if ((NULL == hKDHandle) || (NULL == pcKey) || (NULL == pstData))
    {
        return BS_NULL_PARA;
    }

    pstCtrl = hKDHandle;

    stKey.pcData = pcKey;
    stKey.uiLen = strlen(pcKey);

    pstNodeOld = kd_GetNode(pstCtrl, &stKey);

    eRet = kd_AddKeyValue(pstCtrl, &stKey, pstData);
    if (BS_OK == eRet)
    {
        kd_DelNode(pstCtrl, pstNodeOld);
    }

    return eRet;
}

/* 相比KD_SetKeyData, 它不会为Data申请内存，而是直接将hHandle指针挂在LSTR里面,此时LSTR的uiLen字段为0 */
BS_STATUS KD_SetKeyHandle(IN KD_HANDLE hKDHandle, IN CHAR *pcKey, IN HANDLE hHandle)
{
    _KD_CTRL_S *pstCtrl;
    LSTR_S stKey;
    LSTR_S stValue;
    _KD_NODE_S *pstNodeOld;
    BS_STATUS eRet;

    if ((NULL == hKDHandle) || (NULL == pcKey))
    {
        return BS_NULL_PARA;
    }

    pstCtrl = hKDHandle;

    stKey.pcData = pcKey;
    stKey.uiLen = strlen(pcKey);
    stValue.pcData = hHandle;
    stValue.uiLen = 0;

    pstNodeOld = kd_GetNode(pstCtrl, &stKey);

    eRet = kd_AddKeyValue(pstCtrl, &stKey, &stValue);
    if (BS_OK == eRet)
    {
        kd_DelNode(pstCtrl, pstNodeOld);
    }

    return eRet;
}

HANDLE KD_GetKeyHandle(IN KD_HANDLE hKDHandle, IN CHAR *pcKey)
{
    LSTR_S *pstLstr;

    pstLstr = KD_GetKeyData(hKDHandle, pcKey);
    if (NULL == pstLstr)
    {
        return NULL;
    }

    return pstLstr->pcData;
}

KD_S * KD_GetNext(IN KD_HANDLE hKDHandle, IN KD_S *pstCurrent)
{
    _KD_CTRL_S *pstCtrl;
    _KD_NODE_S *pstNodeTmp = NULL;

    pstCtrl = hKDHandle;

    if ((NULL == pstCurrent) || (pstCurrent->pcKey == NULL) || (pstCurrent->pcKey[0] == '\0'))
    {
        pstNodeTmp = DLL_FIRST(&pstCtrl->stKeyDataList);
        if (NULL == pstNodeTmp)
        {
            return NULL;
        }
        return &pstNodeTmp->stKd;
    }

    DLL_SCAN(&pstCtrl->stKeyDataList, pstNodeTmp)
    {
        if (strcmp(pstNodeTmp->stKd.pcKey, pstCurrent->pcKey) > 0)
        {
            return &pstNodeTmp->stKd;
        }
    }

    return NULL;
}

VOID KD_DelKey(IN KD_HANDLE hKDHandle, IN CHAR *pcKey)
{
    _KD_CTRL_S *pstCtrl;
    _KD_NODE_S *pstNode;
    LSTR_S stKey;

    pstCtrl = hKDHandle;

    stKey.pcData = pcKey;
    stKey.uiLen = strlen(pcKey);

    pstNode = kd_GetNode(pstCtrl, &stKey);
    if (NULL == pstNode)
    {
        return;
    }

    kd_DelNode(pstCtrl, pstNode);
}

VOID KD_Walk(IN KD_HANDLE hKDHandle, IN PF_KD_WALK_FUNC pfFunc, IN HANDLE hUserHandle)
{
    _KD_NODE_S *pstNode;
    _KD_CTRL_S *pstCtrl;

    if ((NULL == hKDHandle) || (NULL == pfFunc))
    {
        return;
    }

    pstCtrl = hKDHandle;

    DLL_SCAN(&pstCtrl->stKeyDataList, pstNode)
    {
        pfFunc(&pstNode->stKd, hUserHandle);
    }
}

