/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-10-14
* Description: 二级表管理. 可以添加List，可以向List中添加Rule
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/nap_utl.h"
#include "utl/list_rule.h"

typedef struct
{
    CHAR *pcListName;
    UINT uiRefCount;
    RULE_LIST_S stRuleList;
}LIST_RULE_LIST_S;

static VOID _listrule_DelListNode
(
    IN LIST_RULE_CTRL_S *pstCtx,
    IN LIST_RULE_LIST_S *pstList,
    IN PF_RULE_FREE pfFunc,
    IN VOID *pUserHandle
)
{
    RuleList_Finit(&pstList->stRuleList, pfFunc, pUserHandle);
    MEM_Free(pstList->pcListName);
    NAP_Free(pstCtx->hListNap, pstList);
}

BS_STATUS ListRule_Init(INOUT LIST_RULE_CTRL_S *listrule)
{
    listrule->hListNap = NAP_Create(NAP_TYPE_HASH, 0, sizeof(LIST_RULE_LIST_S), 0);
    if (NULL == listrule->hListNap) {
        RETURN(BS_NO_MEMORY);
    }

    return BS_OK;
}

void ListRule_Finit(INOUT LIST_RULE_CTRL_S *listrule, PF_RULE_FREE pfFunc, VOID *pUserHandle)
{
    LIST_RULE_LIST_S *pstList;
    UINT uiIndex = NAP_INVALID_INDEX;

    while ((uiIndex = NAP_GetNextIndex(listrule->hListNap, uiIndex))
            != NAP_INVALID_INDEX) {
        pstList = NAP_GetNodeByIndex(listrule->hListNap, uiIndex);
        _listrule_DelListNode(listrule, pstList, pfFunc, pUserHandle);
    }

    NAP_Destory(listrule->hListNap);
}

LIST_RULE_HANDLE ListRule_Create()
{
    LIST_RULE_CTRL_S *pstCtrl;

    pstCtrl = MEM_ZMalloc(sizeof(LIST_RULE_CTRL_S));
    if (NULL == pstCtrl) {
        return NULL;
    }

    if (BS_OK != ListRule_Init(pstCtrl)) {
        MEM_Free(pstCtrl);
        return NULL;
    }

    return pstCtrl;
}

VOID ListRule_Destroy
(
    IN LIST_RULE_HANDLE hListRule,
    IN PF_RULE_FREE pfFunc,
    IN VOID *pUserHandle
)
{
    LIST_RULE_CTRL_S *pstCtx = hListRule;

    ListRule_Finit(pstCtx, pfFunc, pUserHandle);
    MEM_Free(pstCtx);

    return;
}

UINT ListRule_AddList(IN LIST_RULE_HANDLE hListRule, IN CHAR *pcListName)
{
    LIST_RULE_CTRL_S *pstCtx = hListRule;
    LIST_RULE_LIST_S *pstList;
    CHAR *pcListNameDup;

    pcListNameDup = TXT_Strdup(pcListName);
    if (NULL == pcListNameDup)
    {
        return 0;
    }

    pstList = NAP_ZAlloc(pstCtx->hListNap);
    if (NULL == pstList)
    {
        MEM_Free(pcListNameDup);
        return 0;
    }

    RuleList_Init(&pstList->stRuleList);

    pstList->pcListName = pcListNameDup;

    return NAP_GetIDByNode(pstCtx->hListNap, pstList);
}

VOID ListRule_DelList
(
    IN LIST_RULE_HANDLE hListRule,
    IN UINT uiListID,
    IN PF_RULE_FREE pfFunc,
    IN VOID *pUserHandle
)
{
    LIST_RULE_CTRL_S *pstCtx = hListRule;
    LIST_RULE_LIST_S *pstList;

    pstList = NAP_GetNodeByID(pstCtx->hListNap, uiListID);
    if (NULL == pstList)
    {
        return;
    }

    _listrule_DelListNode(pstCtx, pstList, pfFunc, pUserHandle);
}

/* 增加List的引用计数 */
BS_STATUS ListRule_AddListRef(IN LIST_RULE_HANDLE hListRule, IN UINT uiListID)
{
    LIST_RULE_CTRL_S *pstCtx = hListRule;
    LIST_RULE_LIST_S *pstList;

    pstList = NAP_GetNodeByID(pstCtx->hListNap, uiListID);
    if (NULL == pstList)
    {
        return BS_NOT_FOUND;
    }

    pstList->uiRefCount ++;

    return BS_OK;
}

/* 减少List的引用计数 */
BS_STATUS ListRule_DelListRef(IN LIST_RULE_HANDLE hListRule, IN UINT uiListID)
{
    LIST_RULE_CTRL_S *pstCtx = hListRule;
    LIST_RULE_LIST_S *pstList;

    pstList = NAP_GetNodeByID(pstCtx->hListNap, uiListID);
    if (NULL == pstList)
    {
        return BS_NOT_FOUND;
    }

    pstList->uiRefCount --;

    return BS_OK;
}

/* 获取List的引用计数 */
UINT ListRule_GetListRef(IN LIST_RULE_HANDLE hListRule, IN UINT uiListID)
{
    LIST_RULE_CTRL_S *pstCtx = hListRule;
    LIST_RULE_LIST_S *pstList;

    pstList = NAP_GetNodeByID(pstCtx->hListNap, uiListID);
    if (NULL == pstList)
    {
        return 0;
    }

    return pstList->uiRefCount;
}

UINT ListRule_FindListByName(IN LIST_RULE_HANDLE hListRule, IN CHAR *pcListName)
{
    LIST_RULE_CTRL_S *pstCtx = hListRule;
    LIST_RULE_LIST_S *pstList;
    UINT uiIndex = NAP_INVALID_INDEX;

    while ((uiIndex = NAP_GetNextIndex(pstCtx->hListNap, uiIndex))
            != NAP_INVALID_INDEX) {
        pstList = NAP_GetNodeByIndex(pstCtx->hListNap, uiIndex);
        if (strcmp(pstList->pcListName, pcListName) == 0) {
            return NAP_GetIDByNode(pstCtx->hListNap, pstList);
        }
    }

    return 0;
}

UINT ListRule_GetNextList(IN LIST_RULE_HANDLE hListRule, IN UINT ulCurrentListID/* 0表示获取第一个 */)
{
    LIST_RULE_CTRL_S *pstCtx = hListRule;
    
    return NAP_GetNextID(pstCtx->hListNap, ulCurrentListID);
}

CHAR * ListRule_GetListNameByID(IN LIST_RULE_HANDLE hListRule, IN UINT ulListID)
{
    LIST_RULE_CTRL_S *pstCtx = hListRule;
    LIST_RULE_LIST_S *pstList;

    pstList = NAP_GetNodeByID(pstCtx->hListNap, ulListID);
    if (NULL == pstList)
    {
        return NULL;
    }

    return pstList->pcListName;
}

BS_STATUS ListRule_AddRule
(
    IN LIST_RULE_HANDLE hCtx,
    IN UINT uiListID,
    IN UINT uiRuleID,
    IN RULE_NODE_S *pstRule
)
{
    LIST_RULE_CTRL_S *pstCtx = hCtx;
    LIST_RULE_LIST_S *pstList;

    if (uiRuleID == 0)
    {
        BS_DBGASSERT(0 != uiRuleID);
        return BS_BAD_PARA;
    }

    pstList = NAP_GetNodeByID(pstCtx->hListNap, uiListID);
    if (NULL == pstList)
    {
        return BS_NO_SUCH;
    }

    RuleList_Add(&pstList->stRuleList, uiRuleID, pstRule);

    return BS_OK;
}

/* 查找并从表上摘掉Rule, 返回找到的Rule指针 */
RULE_NODE_S * ListRule_DelRule(IN LIST_RULE_HANDLE hCtx, IN UINT uiListID, IN UINT uiRuleID)
{
    LIST_RULE_CTRL_S *pstCtx = hCtx;
    LIST_RULE_LIST_S *pstList;

    pstList = NAP_GetNodeByID(pstCtx->hListNap, uiListID);
    if (NULL == pstList)
    {
        return NULL;
    }

    return RuleList_Del(&pstList->stRuleList, uiRuleID);
}

/* 按顺序重新安排所有rule的ID号 */
VOID ListRule_ResetRuleID(IN LIST_RULE_HANDLE hCtx, IN UINT uiListID, IN UINT step /* 两条表项间的间隔 */)
{
    LIST_RULE_CTRL_S *pstCtx = hCtx;
    LIST_RULE_LIST_S *pstList;

    if (step == 0) {
        step = 1;
    }

    pstList = NAP_GetNodeByID(pstCtx->hListNap, uiListID);
    if (NULL == pstList) {
        return;
    }

    RuleList_ResetRuleID(&pstList->stRuleList, step);
}

RULE_NODE_S * ListRule_GetRule(IN LIST_RULE_HANDLE hCtx, IN UINT uiListID, IN UINT uiRuleID)
{
    LIST_RULE_CTRL_S *pstCtx = hCtx;
    LIST_RULE_LIST_S *pstList;

    pstList = NAP_GetNodeByID(pstCtx->hListNap, uiListID);
    if (NULL == pstList)
    {
        return NULL;
    }

    return RuleList_GetRule(&pstList->stRuleList, uiRuleID);
}

/* 移动rule */
BS_STATUS ListRule_MoveRule
(
    IN LIST_RULE_HANDLE hCtx,
    IN UINT uiListID,
    IN UINT uiOldRuleID,
    IN UINT uiNewRuleID
)
{
    LIST_RULE_CTRL_S *pstCtx = hCtx;
    LIST_RULE_LIST_S *pstList;

    pstList = NAP_GetNodeByID(pstCtx->hListNap, uiListID);
    if (NULL == pstList)
    {
        return BS_NOT_FOUND;
    }

    RuleList_MoveRule(&pstList->stRuleList, uiOldRuleID, uiNewRuleID);

    return BS_OK;
}

RULE_NODE_S * ListRule_GetNextRule
(
    IN LIST_RULE_HANDLE hCtx,
    IN UINT uiListID,
    IN UINT uiCurrentRuleID/* RULE_ID_INVALID 表示从头开始 */
)
{
    LIST_RULE_CTRL_S *pstCtx = hCtx;
    LIST_RULE_LIST_S *pstList;

    pstList = NAP_GetNodeByID(pstCtx->hListNap, uiListID);
    if (NULL == pstList)
    {
        return NULL;
    }

    return RuleList_GetNextByID(&pstList->stRuleList, uiCurrentRuleID);
}

