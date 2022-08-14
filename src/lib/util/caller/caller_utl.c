/******************************************************************************
* Copyright (C),    LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2014-1-6
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/caller_utl.h"

#define _CALLER_DBG_FLAG 0x1

typedef struct
{
    DLL_NODE_S stNode;
    UINT uiPRI;
    CHAR *pcName;
    PF_CALLER_FUNC pfFunc;
    USER_HANDLE_S stUserHandle;
}_CALLER_NODE_S;

typedef struct
{
    DLL_HEAD_S stFuncsList;
    _CALLER_NODE_S *pstCurNode; /* 当前调用到了哪个Node. 应该执行下一个了 */
    UINT uiDbgFlag;
}_CALLER_CTRL_S;

static VOID caller_FreeEach(IN VOID *pNode)
{
    _CALLER_NODE_S *pstNode = pNode;
    
    MEM_Free(pstNode);
}

static CALLER_RET_E caller_Call(IN _CALLER_CTRL_S *pstCtrl)
{
    _CALLER_NODE_S *pstNode;
    CALLER_RET_E eRet;

    while ((pstNode = DLL_NEXT(&pstCtrl->stFuncsList, pstCtrl->pstCurNode)) != NULL)
    {
        pstCtrl->pstCurNode = pstNode;

        BS_DBG_OUTPUT(pstCtrl->uiDbgFlag, _CALLER_DBG_FLAG, ("Caller: Call %s...", pstNode->pcName));

        eRet = pstNode->pfFunc(&pstNode->stUserHandle);
        if (eRet == CALLER_RET_ERR)
        {
            BS_DBG_OUTPUT(pstCtrl->uiDbgFlag, _CALLER_DBG_FLAG, ("Error.\r\n"));
            return eRet;
        }
        if (eRet == CALLER_RET_INCOMPLETE)
        {
            BS_DBG_OUTPUT(pstCtrl->uiDbgFlag, _CALLER_DBG_FLAG, ("Incomplete..."));
            return eRet;
        }

        BS_DBG_OUTPUT(pstCtrl->uiDbgFlag, _CALLER_DBG_FLAG, ("OK.\r\n"));
    }

    pstCtrl->pstCurNode = NULL;

    return CALLER_RET_OK;
}

static _CALLER_NODE_S * caller_FindByName(IN _CALLER_CTRL_S *pstCtrl, IN CHAR *pcName)
{
    _CALLER_NODE_S *pstNode;

    DLL_SCAN(&pstCtrl->stFuncsList,pstNode)
    {
        if (strcmp(pcName, pstNode->pcName) == 0)
        {
            return pstNode;
        }
    }

    return NULL;
}

HANDLE Caller_Create()
{
    _CALLER_CTRL_S *pstCtrl;

    pstCtrl = MEM_ZMalloc(sizeof(_CALLER_CTRL_S));
    if (NULL == pstCtrl)
    {
        return NULL;
    }

    DLL_INIT(&pstCtrl->stFuncsList);

    return pstCtrl;
}

VOID Caller_Destory(IN HANDLE hCaller)
{
    _CALLER_CTRL_S *pstCtrl = hCaller;

    DLL_FREE(&pstCtrl->stFuncsList, caller_FreeEach);
    MEM_Free(pstCtrl);
}

BS_STATUS Caller_Add
(
    IN HANDLE hCaller,
    IN CHAR *pcName, /* 名字 */
    IN UINT uiPRI/* 越小优先级越高 */,
    IN PF_CALLER_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    _CALLER_NODE_S *pstNode;
    _CALLER_NODE_S *pstNodeTmp;
    _CALLER_CTRL_S *pstCtrl = hCaller;

    if (NULL != caller_FindByName(pstCtrl, pcName))
    {
        return BS_ALREADY_EXIST;
    }

    pstNode = MEM_ZMalloc(sizeof(_CALLER_NODE_S));
    if (NULL == pstNode)
    {
        return BS_NO_MEMORY;
    }

    pstNode->uiPRI = uiPRI;
    pstNode->pfFunc = pfFunc;
    pstNode->pcName = pcName;
    if (NULL != pstUserHandle)
    {
        pstNode->stUserHandle = *pstUserHandle;
    }

    DLL_SCAN(&pstCtrl->stFuncsList, pstNodeTmp)
    {
        if (pstNodeTmp->uiPRI >= uiPRI)
        {
            break;
        }
    }

    if (pstNodeTmp == NULL)
    {
        DLL_ADD(&pstCtrl->stFuncsList, pstNode);
    }
    else
    {
        DLL_INSERT_BEFORE(&pstCtrl->stFuncsList, pstNode, pstNodeTmp);
    }

    return BS_OK;
}

CALLER_RET_E Caller_Call(IN HANDLE hCaller)
{
    _CALLER_CTRL_S *pstCtrl = hCaller;

    return caller_Call(pstCtrl);
}

/* 一个节点异步执行完后,调用此接口通知完成 */
CALLER_RET_E Caller_Finished(IN HANDLE hCaller)
{
    _CALLER_CTRL_S *pstCtrl = hCaller;

    BS_DBG_OUTPUT(pstCtrl->uiDbgFlag, _CALLER_DBG_FLAG, ("Finished.\r\n"));

    return caller_Call(pstCtrl);
}

/* 将调用阶段设置到指定Name的位置 */
BS_STATUS Caller_SetByName(IN HANDLE hCaller, IN CHAR *pcName)
{
    _CALLER_CTRL_S *pstCtrl = hCaller;
    _CALLER_NODE_S *pstNode;

    pstNode = caller_FindByName(pstCtrl, pcName);
    if (NULL == pstNode)
    {
        return BS_ERR;
    }

    pstCtrl->pstCurNode = DLL_PREV(&pstCtrl->stFuncsList, pstNode);

    return BS_OK;
}

/* 将调用阶段设置到开始位置 */
VOID Caller_Reset(IN HANDLE hCaller)
{
    _CALLER_CTRL_S *pstCtrl = hCaller;

    pstCtrl->pstCurNode = NULL;
}

VOID Caller_SetDbg(IN HANDLE hCaller)
{
    _CALLER_CTRL_S *pstCtrl = hCaller;

    pstCtrl->uiDbgFlag = _CALLER_DBG_FLAG;
}

VOID Caller_ClrDbg(IN HANDLE hCaller)
{
    _CALLER_CTRL_S *pstCtrl = hCaller;

    pstCtrl->uiDbgFlag = 0;
}

