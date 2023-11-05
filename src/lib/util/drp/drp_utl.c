/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-7-24
* Description: deep replace   深度替换
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/drp_utl.h"

#include "drp_def.h"

DRP_HANDLE DRP_Create(IN CHAR *pcStartTag, IN CHAR *pcEndTag)
{
    DRP_CTRL_S *pstCtrl;

    if ((NULL == pcStartTag) || (NULL == pcEndTag))
    {
        return NULL;
    }

    pstCtrl = MEM_ZMalloc(sizeof(DRP_CTRL_S));
    if (NULL == pstCtrl)
    {
        return NULL;
    }

    pstCtrl->hMemPool = MEMPOOL_Create(0);
    if (NULL == pstCtrl->hMemPool)
    {
        DRP_Destory(pstCtrl);
        return NULL;
    }

    pstCtrl->pcStartTag = TXT_Strdup(pcStartTag);
    pstCtrl->pcEndTag = TXT_Strdup(pcEndTag);
    if ((NULL == pstCtrl->pcStartTag) || (NULL == pstCtrl->pcEndTag))
    {
        DRP_Destory(pstCtrl);
        return NULL;
    }

    pstCtrl->uiStartTagLen = strlen(pcStartTag);
    pstCtrl->uiEndTagLen = strlen(pcEndTag);

    Sunday_ComplexPatt((void*)pcStartTag, strlen(pcStartTag), &pstCtrl->stSundaySkipStart);
    Sunday_ComplexPatt((void*)pcEndTag, strlen(pcEndTag), &pstCtrl->stSundaySkipEnd);

    DLL_INIT(&pstCtrl->stKeyList);

    return pstCtrl;
}

VOID DRP_Destory(IN DRP_HANDLE hDrp)
{
    DRP_CTRL_S *pstCtrl = hDrp;

    if (NULL != pstCtrl)
    {
        if (NULL != pstCtrl->pcStartTag)
        {
            MEM_Free(pstCtrl->pcStartTag);
        }
        if (NULL != pstCtrl->pcEndTag)
        {
            MEM_Free(pstCtrl->pcEndTag);
        }
        if (NULL != pstCtrl->hMemPool)
        {
            MEMPOOL_Destory(pstCtrl->hMemPool);
        }
        MEM_Free(pstCtrl);
    }
}

static DRP_NODE_S * drp_Find(IN DRP_CTRL_S *pstCtrl, IN CHAR *pcKey, IN UINT uiKeyLen)
{
    DRP_NODE_S *pstNode;

    DLL_SCAN(&pstCtrl->stKeyList, pstNode)
    {
        if ((uiKeyLen == pstNode->uiKeyLen) && (strncmp(pcKey, pstNode->pcKey, uiKeyLen) == 0))
        {
            return pstNode;
        }
    }

    return NULL;
}

static DRP_NODE_S * drp_Add(IN DRP_CTRL_S *pstCtrl, IN CHAR *pcKey)
{
    DRP_NODE_S *pstNode;

    pstNode = MEMPOOL_ZAlloc(pstCtrl->hMemPool, sizeof(DRP_NODE_S));
    if (NULL == pstNode)
    {
        return NULL;
    }

    pstNode->uiKeyLen = strlen(pcKey);
    pstNode->pcKey = pcKey;

    DLL_ADD(&pstCtrl->stKeyList, pstNode);

    return pstNode;
}

DRP_NODE_S * DRP_Find(IN DRP_HANDLE hDrp, IN CHAR *pcKey, IN UINT uiKeyLen)
{
    DRP_NODE_S *pstNode;
    DRP_CTRL_S *pstCtrl = hDrp;

    pstNode = drp_Find(pstCtrl, pcKey, uiKeyLen);
    if (NULL == pstNode)
    {
        return NULL;
    }

    return pstNode;
}

BS_STATUS DRP_Set(IN DRP_HANDLE hDrp, IN CHAR *pcKey, IN PF_DRP_SOURCE_FUNC pfFunc, IN HANDLE hUserHandle2)
{
    DRP_CTRL_S *pstCtrl = hDrp;
    DRP_NODE_S *pstNode;

    pstNode = drp_Find(pstCtrl, pcKey, strlen(pcKey));

    if (NULL == pstNode)
    {
        pstNode = drp_Add(pstCtrl, pcKey);
        if (NULL == pstNode)
        {
            return BS_NO_MEMORY;
        }
    }

    pstNode->pfSourceFunc = pfFunc;
    pstNode->hUserHandle2 = hUserHandle2;

    return BS_OK;
}

BS_STATUS DRP_CtxOutput(IN VOID *pDrpCtx, IN void *data, IN UINT uiDataLen)
{
    DRP_CTX_S *pstCtx = pDrpCtx;

    if (NULL == pDrpCtx)
    {
        BS_DBGASSERT(0);
        return BS_NULL_PARA;
    }

    if ((NULL == data) || (uiDataLen == 0))
    {
        return BS_OK;
    }

    return pstCtx->pfCtxOutput(pDrpCtx, data, uiDataLen);
}


