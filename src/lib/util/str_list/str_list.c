/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-8-30
* Description: 字符串链表
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/lstr_utl.h"
#include "utl/str_list.h"

typedef struct
{
    DLL_HEAD_S stList;
    UINT uiFlag;
}STRLIST_S;

static INT _strlist_Cmp(STRLIST_S *pstStrList, IN LSTR_S *pstLstr1, IN LSTR_S *pstLstr2)
{
    if (pstStrList->uiFlag & STRLIST_FLAG_CASE_SENSITIVE)
    {
        return LSTR_Cmp(pstLstr1, pstLstr2);
    }

    return LSTR_CaseCmp(pstLstr1, pstLstr2);
}

static STRLIST_NODE_S *_strlist_FindLstr(IN STRLIST_S *pstStrList, IN LSTR_S *pstLstr)
{
    STRLIST_NODE_S *pstNode;
    
    DLL_SCAN(&pstStrList->stList, pstNode)
    {
        if (0 == _strlist_Cmp(pstStrList, &pstNode->stStr, pstLstr))
        {
            return pstNode;
        }
    }

    return NULL;
}

static STRLIST_NODE_S *_strlist_Find(IN STRLIST_S *pstStrList, IN CHAR *pcStr)
{
    LSTR_S stStr;

    stStr.pcData = pcStr;
    stStr.uiLen = strlen(pcStr);
    
    return _strlist_FindLstr(pstStrList, &stStr);
}

STRLIST_HANDLE StrList_Create(IN UINT uiFlag)
{
    STRLIST_S *pstStrList;

    pstStrList = MEM_ZMalloc(sizeof(STRLIST_S));
    if (NULL == pstStrList)
    {
        return NULL;
    }

    DLL_INIT(&pstStrList->stList);
    pstStrList->uiFlag = uiFlag;

    return pstStrList;
}

BS_STATUS StrList_Add(IN STRLIST_HANDLE hStrList, IN CHAR *pcStr)
{
    STRLIST_S *pstStrList = hStrList;
    STRLIST_NODE_S *pstNode;

    if (NULL == pstStrList)
    {
        return BS_NULL_PARA;
    }

    if (pstStrList->uiFlag & STRLIST_FLAG_CHECK_REPEAT)
    {
        if (NULL != _strlist_Find(pstStrList, pcStr))
        {
            return BS_ALREADY_EXIST;
        }
    }

    pstNode = MEM_ZMalloc(sizeof(STRLIST_NODE_S));
    if (NULL == pstNode)
    {
        return BS_NO_MEMORY;
    }

    pstNode->stStr.pcData = TXT_Strdup(pcStr);
    if (pstNode->stStr.pcData == NULL)
    {
        MEM_Free(pstNode);
        return BS_NO_MEMORY;
    }

    pstNode->stStr.uiLen = strlen(pcStr);

    DLL_ADD(&pstStrList->stList, pstNode);

    return BS_OK;
}

VOID StrList_Del(IN STRLIST_HANDLE hStrList, IN CHAR *pcStr)
{
    STRLIST_S *pstStrList = hStrList;
    STRLIST_NODE_S *pstNode, *pstNodeNext;
    LSTR_S stStr;

    stStr.pcData = pcStr;
    stStr.uiLen = strlen(pcStr);

    DLL_SAFE_SCAN(&pstStrList->stList, pstNode, pstNodeNext)
    {
        if (0 == _strlist_Cmp(pstStrList, &stStr, &pstNode->stStr))
        {
            DLL_DEL(&pstStrList->stList, pstNode);
            MEM_Free(pstNode->stStr.pcData);
            MEM_Free(pstNode);
        }
    }
}

CHAR * StrList_Find(IN STRLIST_HANDLE hStrList, IN CHAR *pcStr)
{
    STRLIST_S *pstStrList = hStrList;
    STRLIST_NODE_S *pstNode;

    if (NULL == pstStrList)
    {
        return NULL;
    }

    pstNode = _strlist_Find(pstStrList, pcStr);
    if (NULL != pstNode)
    {
        return pstNode->stStr.pcData;
    }

    return NULL;
}

CHAR * StrList_FindByLstr(IN STRLIST_HANDLE hStrList, IN LSTR_S *pstLstr)
{
    STRLIST_S *pstStrList = hStrList;
    STRLIST_NODE_S *pstNode;

    if (NULL == pstStrList)
    {
        return NULL;
    }

    pstNode = _strlist_FindLstr(pstStrList, pstLstr);
    if (NULL != pstNode)
    {
        return pstNode->stStr.pcData;
    }

    return NULL;
}

STRLIST_NODE_S * StrList_GetNext(IN STRLIST_HANDLE hStrList, IN STRLIST_NODE_S *pstCurr)
{
    STRLIST_S *pstStrList = hStrList;
    
    if (pstCurr == NULL)
    {
        return DLL_FIRST(&pstStrList->stList);
    }

    return DLL_NEXT(&pstStrList->stList, pstCurr);
}


