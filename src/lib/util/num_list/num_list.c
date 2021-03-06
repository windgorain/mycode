/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-8-31
* Description: Number List: 支持以字符串形式描述的数字列表
*              格式例如: 6,2-5,8-7.
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/lstr_utl.h"
#include "utl/num_list.h"


VOID NumList_Init(IN NUM_LIST_S *pstList)
{
    DLL_INIT(&pstList->stList);
}

VOID NumList_Finit(IN NUM_LIST_S *pstList)
{
    NUM_LIST_NODE_S *pstNode, *pstNodeTmp;

    DLL_SAFE_SCAN(&pstList->stList, pstNode, pstNodeTmp)
    {
        DLL_DEL(&pstList->stList, pstNode);
        MEM_Free(pstNode);
    }
}

static int _numlist_ParseElement(IN NUM_LIST_S *pstList, IN LSTR_S *pstEle)
{
    LSTR_S stBegin;
    LSTR_S stEnd;
    UINT uiNumBegin;
    UINT uiNumEnd;

    LSTR_Split(pstEle, '-', &stBegin, &stEnd);

    LSTR_Strim(&stBegin, " \t\r\n", &stBegin);
    LSTR_Strim(&stEnd, " \t\r\n", &stEnd);
    if (stBegin.uiLen == 0)
    {
        return 0;
    }

    if (BS_OK != LSTR_Atoui(&stBegin, &uiNumBegin))
    {
        RETURN(BS_ERR);
    }

    if (stEnd.uiLen == 0)
    {
        return NumList_AddRange(pstList, uiNumBegin, uiNumBegin);
    }

    if (BS_OK != LSTR_Atoui(&stEnd, &uiNumEnd))
    {
        RETURN(BS_ERR);
    }

    return NumList_AddRange(pstList, uiNumBegin, uiNumEnd);
}

BS_STATUS NumList_AddRange(IN NUM_LIST_S *pstList, IN INT iBegin, IN INT iEnd)
{
    NUM_LIST_NODE_S *pstNode;

    pstNode = MEM_ZMalloc(sizeof(NUM_LIST_NODE_S));
    if (NULL == pstNode)
    {
        return BS_NO_MEMORY;
    }

    pstNode->iNumBegin = iBegin;
    pstNode->iNumEnd = iEnd;

    DLL_ADD(&pstList->stList, pstNode);

    return BS_OK;
}

/* 删除一个Range, 只查找完全匹配的第一个节点删除 */
BS_STATUS NumList_DelRange(IN NUM_LIST_S *pstList, IN INT iBegin, IN INT iEnd)
{
    NUM_LIST_NODE_S *pstNode;

    DLL_SCAN(&pstList->stList, pstNode)
    {
        if ((pstNode->iNumBegin == iBegin) && (pstNode->iNumEnd == iEnd))
        {
            DLL_DEL(&pstList->stList, pstNode);
            return BS_OK;
        }
    }

    return BS_OK;
}

BS_STATUS NumList_ParseLstr(IN NUM_LIST_S *pstList, IN LSTR_S *pstNumListString)
{
    LSTR_S stElement;
    BS_STATUS eRet = BS_OK;
    
    LSTR_SCAN_ELEMENT_BEGIN(pstNumListString->pcData, pstNumListString->uiLen, ',', &stElement)
    {
        eRet |= _numlist_ParseElement(pstList, &stElement);
    }LSTR_SCAN_ELEMENT_END();

	return eRet;
}

BS_STATUS NumList_ParseStr(IN NUM_LIST_S *pstList, IN char *str)
{
    LSTR_S lstr;

    lstr.pcData = str;
    lstr.uiLen = strlen(str);

    return NumList_ParseLstr(pstList, &lstr);
}

/* 将两个List串成一个 */
VOID NumList_Cat(INOUT NUM_LIST_S *pstListDst, INOUT NUM_LIST_S *pstListSrc)
{
    DLL_Cat(&pstListDst->stList, &pstListSrc->stList);
}

static INT _numlist_Cmp(IN DLL_NODE_S *pstDllNode1, IN DLL_NODE_S *pstDllNode2, IN HANDLE hHandle)
{
    NUM_LIST_NODE_S *pstNode1 = (VOID*)pstDllNode1;
    NUM_LIST_NODE_S *pstNode2 = (VOID*)pstDllNode2;

    if (pstNode1->iNumBegin > pstNode2->iNumBegin)
    {
        return 1;
    }

    if (pstNode1->iNumBegin < pstNode2->iNumBegin)
    {
        return -1;
    }

    if (pstNode1->iNumEnd < pstNode2->iNumEnd)
    {
        return 1;
    }

    if (pstNode1->iNumEnd > pstNode2->iNumEnd)
    {
        return -1;
    }

    return 0;
}

/* 对数字进行排序 */
VOID NumList_Sort(IN NUM_LIST_S *pstList)
{
    DLL_Sort(&pstList->stList, _numlist_Cmp, 0);
}

/* 将有重叠部分消除 */
VOID NumList_Compress(IN NUM_LIST_S *pstList)
{
    NUM_LIST_NODE_S *pstNode, *pstNodeNext;
    
    /* 1. 进行排序 */
    NumList_Sort(pstList);

    /* 2. 进行消重 */
    pstNode = DLL_FIRST(&pstList->stList);

    if (NULL == pstNode)
    {
        return;
    }

    while(1)
    {
        pstNodeNext = DLL_NEXT(&pstList->stList, pstNode);
        if (NULL == pstNodeNext)
        {
            break;
        }

        if (pstNodeNext->iNumEnd <= pstNode->iNumEnd)
        {
            /* 前一个节点完全包含后面一个节点, 删除此节点 */
            DLL_DEL(&pstList->stList, pstNodeNext);
            MEM_Free(pstNodeNext);
            continue;
        }

        if (pstNodeNext->iNumBegin <= pstNode->iNumBegin)
        {
            pstNodeNext->iNumBegin = pstNode->iNumEnd + 1;
        }

        pstNode = pstNodeNext;
    }

    return;
}

/* 将可以连续的部分进行连续化 */
VOID NumList_Continue(IN NUM_LIST_S *pstList)
{
    NUM_LIST_NODE_S *pstNode, *pstNodeNext;

    /* 先进性排序和消重 */
    NumList_Compress(pstList);

    /* 2. 进行连续化 */
    pstNode = DLL_FIRST(&pstList->stList);

    if (NULL == pstNode)
    {
        return;
    }

    while(1)
    {
        pstNodeNext = DLL_NEXT(&pstList->stList, pstNode);
        if (NULL == pstNodeNext)
        {
            break;
        }

        if (pstNodeNext->iNumBegin == pstNode->iNumEnd + 1)
        {
            pstNode->iNumEnd = pstNodeNext->iNumEnd;
            DLL_DEL(&pstList->stList, pstNodeNext);
            MEM_Free(pstNodeNext);
            continue;
        }

        pstNode = pstNodeNext;
    }

    return;
}

/* 判断一个数字是否在List 内 */
BOOL_T NumList_IsNumInTheList(IN NUM_LIST_S *pstList, IN INT iNum)
{
    INT64 iStart, iEnd;

    NUM_LIST_SCAN_BEGIN(pstList, iStart, iEnd)
    {
        if ((iNum >= iStart) && (iNum <= iEnd))
        {
            return TRUE;
        }
    }NUM_LIST_SCAN_END();

    return FALSE;
}


