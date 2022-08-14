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
#include "utl/mem_cap.h"

LIST_RULE_LIST_S *ListRule_GetListByID(IN LIST_RULE_HANDLE hListRule, IN UINT uiListID)
{
    LIST_RULE_CTRL_S *pstCtx = hListRule;
    LIST_RULE_HEAD_S *pstListHead;

    pstListHead = NAP_GetNodeByID(pstCtx->hListNap, uiListID);
    if (NULL == pstListHead)
    {
        return NULL;
    }

    return pstListHead->pstListRule;
}

static VOID _listrule_DelListNode(
    IN LIST_RULE_CTRL_S *pstCtx,
    IN LIST_RULE_HEAD_S *pstListHead,
    IN PF_RULE_FREE pfFunc,
    IN VOID *pUserHandle)
{
    LIST_RULE_LIST_S *pstList = pstListHead->pstListRule;

    if (NULL != pstList)
    {
        ListRule_DestroyList((LIST_RULE_HANDLE)pstCtx, pstList, pfFunc, pUserHandle);
        pstListHead->pstListRule = NULL;
    }

    NAP_Free(pstCtx->hListNap, pstListHead);
    return;
}

static void _listrule_reset(INOUT LIST_RULE_CTRL_S *pstCtx, PF_RULE_FREE pfFunc, VOID *pUserHandle)
{
    LIST_RULE_HEAD_S *pstListHead;
    UINT uiIndex = NAP_INVALID_INDEX;

    while ((uiIndex = NAP_GetNextIndex(pstCtx->hListNap, uiIndex)) != NAP_INVALID_INDEX)
    {
        pstListHead = NAP_GetNodeByIndex(pstCtx->hListNap, uiIndex);
        _listrule_DelListNode(pstCtx, pstListHead, pfFunc, pUserHandle);
    }
}

BS_STATUS ListRule_Init(INOUT LIST_RULE_CTRL_S *pstCtx, void *memcap)
{
    NAP_PARAM_S param = {0};
    
    param.enType = NAP_TYPE_HASH;
    param.uiNodeSize = sizeof(LIST_RULE_HEAD_S);
    param.memcap = memcap;

    pstCtx->hListNap = NAP_Create(&param);
    if (NULL == pstCtx->hListNap)
    {
        RETURN(BS_NO_MEMORY);
    }
    pstCtx->memcap = memcap;

    return BS_OK;
}

void ListRule_Finit(INOUT LIST_RULE_CTRL_S *pstCtx, PF_RULE_FREE pfFunc, VOID *pUserHandle)
{
    _listrule_reset(pstCtx, pfFunc, pUserHandle);
    NAP_Destory(pstCtx->hListNap);
}

LIST_RULE_HANDLE ListRule_Create(void *memcap)
{
    LIST_RULE_CTRL_S *pstCtx;

    pstCtx = MemCap_ZMalloc(memcap, sizeof(LIST_RULE_CTRL_S));
    if (NULL == pstCtx)
    {
        return NULL;
    }

    if (BS_OK != ListRule_Init(pstCtx, memcap))
    {
        MemCap_Free(memcap, pstCtx);
        return NULL;
    }

    return pstCtx;
}

VOID ListRule_Destroy(IN LIST_RULE_HANDLE hListRule, IN PF_RULE_FREE pfFunc,IN VOID *pUserHandle)
{
    LIST_RULE_CTRL_S *pstCtx = hListRule;

    ListRule_Finit(pstCtx, pfFunc, pUserHandle);
    MemCap_Free(pstCtx->memcap, pstCtx);

    return;
}

void ListRule_Reset(IN LIST_RULE_HANDLE hListRule, IN PF_RULE_FREE pfFunc, IN VOID *pUserHandle)
{
    _listrule_reset(hListRule, pfFunc, pUserHandle);
    return;
}

LIST_RULE_LIST_S *ListRule_CreateList(IN LIST_RULE_HANDLE hListRule, IN CHAR *pcListName)
{
    LIST_RULE_CTRL_S *pstCtx = hListRule;
    LIST_RULE_LIST_S *pstList;
    CHAR *pcListNameDup;

    pcListNameDup = TXT_Strdup(pcListName);
    if (NULL == pcListNameDup)
    {
        return NULL;
    }    

    pstList = MemCap_ZMalloc(pstCtx->memcap, sizeof(LIST_RULE_LIST_S));
    if (NULL == pstList)
    {
        MEM_Free(pcListNameDup);
        return NULL;
    }

    RuleList_Init(&pstList->stRuleList);
    pstList->pcListName = pcListNameDup;
    pstList->default_action = BS_ACTION_UNDEF;
    return pstList;
}

VOID ListRule_DestroyList(IN LIST_RULE_HANDLE hListRule, LIST_RULE_LIST_S *pstList, IN PF_RULE_FREE pfFunc, IN VOID *pUserHandle)
{
    LIST_RULE_CTRL_S *pstCtx = hListRule;

    if (NULL != pstList)
    {
        RuleList_Finit(&pstList->stRuleList, pfFunc, pUserHandle);

        if (NULL != pstList->pcListName)
        {
            MEM_Free(pstList->pcListName);
            pstList->pcListName = NULL;
        }
        MemCap_Free(pstCtx->memcap, pstList);
    }

    return;
}

UINT ListRule_AddList(IN LIST_RULE_HANDLE hListRule, IN CHAR *pcListName)
{
    LIST_RULE_CTRL_S *pstCtx = hListRule;
    LIST_RULE_HEAD_S *pstListHead;
    LIST_RULE_LIST_S *pstList;

    pstList = ListRule_CreateList(hListRule, pcListName);
    if (NULL == pstList)
    {
        return 0;
    }

    pstListHead = NAP_ZAlloc(pstCtx->hListNap);
    if (NULL == pstListHead)
    {
        ListRule_DestroyList(hListRule, pstList, NULL, NULL);
        return 0;
    }

    pstListHead->pstListRule = pstList;
    return NAP_GetIDByNode(pstCtx->hListNap, pstListHead);
}

VOID ListRule_DelList(
    IN LIST_RULE_HANDLE hListRule,
    IN UINT uiListID,
    IN PF_RULE_FREE pfFunc,
    IN VOID *pUserHandle)
{
    LIST_RULE_CTRL_S *pstCtx = hListRule;
    LIST_RULE_HEAD_S *pstListHead;

    pstListHead = NAP_GetNodeByID(pstCtx->hListNap, uiListID);
    if (NULL == pstListHead)
    {
        return;
    }

    _listrule_DelListNode(pstCtx, pstListHead, pfFunc, pUserHandle);
}

/* 返回成功, 自动释放Old List; 返回失败, 需要调用者释放New List */
BS_STATUS ListRule_ReplaceList(IN LIST_RULE_HANDLE hListRule, 
                               IN UINT uiListID, IN PF_RULE_FREE pfFunc, IN VOID *pUserHandle, 
                               IN LIST_RULE_LIST_S *pstListNew)
{
    LIST_RULE_CTRL_S *pstCtx = hListRule;
    LIST_RULE_HEAD_S *pstListHead;
    LIST_RULE_LIST_S *pstListOld;

    pstListHead = NAP_GetNodeByID(pstCtx->hListNap, uiListID);
    if (NULL == pstListHead)
    {
        return BS_NOT_FOUND;
    }

    pstListOld = pstListHead->pstListRule;
    if (NULL != pstListOld)
    {
        pstListNew->uiRefCount = pstListOld->uiRefCount;
        pstListNew->default_action = pstListOld->default_action;
    }
    pstListHead->pstListRule = pstListNew;

    if (NULL != pstListOld)
    {
        ListRule_DestroyList(hListRule, pstListOld, pfFunc, pUserHandle);
    }

    return BS_OK;
}

/* 增加List的引用计数 */
BS_STATUS ListRule_AddListRef(IN LIST_RULE_HANDLE hListRule, IN UINT uiListID)
{
    LIST_RULE_LIST_S *pstList;

    pstList = ListRule_GetListByID(hListRule, uiListID);
    if (NULL == pstList)
    {
        return BS_NOT_FOUND;
    }

    pstList->uiRefCount++;

    return BS_OK;
}

/* 减少List的引用计数 */
BS_STATUS ListRule_DelListRef(IN LIST_RULE_HANDLE hListRule, IN UINT uiListID)
{
    LIST_RULE_LIST_S *pstList;

    pstList = ListRule_GetListByID(hListRule, uiListID);
    if (NULL == pstList)
    {
        return BS_NOT_FOUND;
    }

    pstList->uiRefCount--;

    return BS_OK;
}

/* 获取List的引用计数 */
UINT ListRule_GetListRef(IN LIST_RULE_HANDLE hListRule, IN UINT uiListID)
{
    LIST_RULE_LIST_S *pstList;

    pstList = ListRule_GetListByID(hListRule, uiListID);
    if (NULL == pstList)
    {
        return 0;
    }

    return pstList->uiRefCount;
}

BS_ACTION_E ListRule_GetDefaultActionByID(IN LIST_RULE_HANDLE hListRule, IN UINT uiListID)
{
    LIST_RULE_LIST_S *pstList;

    pstList = ListRule_GetListByID(hListRule, uiListID);
    if (NULL == pstList)
    {
        return BS_ACTION_UNDEF;
    }

    return pstList->default_action;
}

BS_STATUS ListRule_SetDefaultActionByID(IN LIST_RULE_HANDLE hListRule, IN UINT uiListID, BS_ACTION_E enAciton)
{
    LIST_RULE_LIST_S *pstList;

    pstList = ListRule_GetListByID(hListRule, uiListID);
    if (NULL == pstList)
    {
        return BS_NO_SUCH;
    }

    pstList->default_action = enAciton;
    return BS_OK;
}

static LIST_RULE_HEAD_S *_listrule_GetListHeadByName(IN LIST_RULE_HANDLE hListRule, IN CHAR *pcListName)
{
    LIST_RULE_CTRL_S *pstCtx = hListRule;
    NAP_HANDLE hListNap = pstCtx->hListNap;
    LIST_RULE_HEAD_S *pstListHead;
    UINT uiIndex = NAP_INVALID_INDEX;

    while ((uiIndex = NAP_GetNextIndex(hListNap, uiIndex)) != NAP_INVALID_INDEX)
    {
        pstListHead = NAP_GetNodeByIndex(hListNap, uiIndex);
        if ((NULL != pstListHead->pstListRule) && (strcmp(pstListHead->pstListRule->pcListName, pcListName) == 0))
        {
            return pstListHead;
        }
    }

    return NULL;
}

UINT ListRule_GetListIDByName(IN LIST_RULE_HANDLE hListRule, IN CHAR *pcListName)
{
    LIST_RULE_CTRL_S *pstCtx = hListRule;
    LIST_RULE_HEAD_S *pstListHead;

    pstListHead = _listrule_GetListHeadByName(hListRule, pcListName);
    if (NULL != pstListHead)
    {
        return NAP_GetIDByNode(pstCtx->hListNap, pstListHead);
    }

    return 0;
}

LIST_RULE_LIST_S *ListRule_GetListByName(IN LIST_RULE_HANDLE hListRule, IN CHAR *pcListName)
{
    LIST_RULE_HEAD_S *pstListHead;

    pstListHead = _listrule_GetListHeadByName(hListRule, pcListName);
    if (NULL != pstListHead)
    {
        return pstListHead->pstListRule;
    }

    return NULL;
}

UINT ListRule_GetNextList(IN LIST_RULE_HANDLE hListRule, IN UINT ulCurrentListID /* 0表示获取第一个 */)
{
    LIST_RULE_CTRL_S *pstCtx = hListRule;

    return NAP_GetNextID(pstCtx->hListNap, ulCurrentListID);
}

CHAR *ListRule_GetListNameByID(IN LIST_RULE_HANDLE hListRule, IN UINT ulListID)
{
    LIST_RULE_LIST_S *pstList;

    pstList = ListRule_GetListByID(hListRule, ulListID);
    if (NULL == pstList)
    {
        return NULL;
    }

    return pstList->pcListName;
}

BS_STATUS ListRule_AddRule2List(LIST_RULE_LIST_S *pstList, IN UINT uiRuleID, IN RULE_NODE_S *pstRule)
{
    if (uiRuleID == 0)
    {
        BS_DBGASSERT(0 != uiRuleID);
        return BS_BAD_PARA;
    }

    RuleList_Add(&pstList->stRuleList, uiRuleID, pstRule);

    return BS_OK;
}

BS_STATUS ListRule_AddRule(IN LIST_RULE_HANDLE hCtx, IN UINT uiListID, IN UINT uiRuleID, IN RULE_NODE_S *pstRule)
{
    LIST_RULE_LIST_S *pstList;

    pstList = ListRule_GetListByID(hCtx, uiListID);
    if (NULL == pstList)
    {
        return BS_NO_SUCH;
    }

    return ListRule_AddRule2List(pstList, uiRuleID, pstRule);
}

/* 查找并从表上摘掉Rule, 返回找到的Rule指针 */
RULE_NODE_S *ListRule_DelRule(IN LIST_RULE_HANDLE hCtx, IN UINT uiListID, IN UINT uiRuleID)
{
    LIST_RULE_LIST_S *pstList;

    pstList = ListRule_GetListByID(hCtx, uiListID);
    if (NULL == pstList)
    {
        return NULL;
    }

    return RuleList_Del(&pstList->stRuleList, uiRuleID);
}

/* 按顺序重新安排所有rule的ID号 */
VOID ListRule_ResetRuleID(IN LIST_RULE_HANDLE hCtx, IN UINT uiListID, IN UINT step /* 两条表项间的间隔 */)
{
    LIST_RULE_LIST_S *pstList;

    if (step == 0)
    {
        step = 1;
    }

    pstList = ListRule_GetListByID(hCtx, uiListID);
    if (NULL == pstList)
    {
        return;
    }

    RuleList_ResetRuleID(&pstList->stRuleList, step);
}

RULE_NODE_S *ListRule_GetRule(IN LIST_RULE_HANDLE hCtx, IN UINT uiListID, IN UINT uiRuleID)
{
    LIST_RULE_LIST_S *pstList;

    pstList = ListRule_GetListByID(hCtx, uiListID);
    if (NULL == pstList)
    {
        return NULL;
    }

    return RuleList_GetRule(&pstList->stRuleList, uiRuleID);
}

RULE_NODE_S *ListRule_GetLastRule(IN LIST_RULE_HANDLE hCtx, IN UINT uiListID)
{
    LIST_RULE_LIST_S *pstList;

    pstList = ListRule_GetListByID(hCtx, uiListID);
    if (NULL == pstList)
    {
        return NULL;
    }

    return RuleList_GetLastRule(&pstList->stRuleList);
}

BS_STATUS ListRule_IncreaseID(IN LIST_RULE_HANDLE hCtx, IN UINT uiListID, IN UINT uiStart, IN UINT uiEnd, IN UINT uiStep)
{
    LIST_RULE_LIST_S *pstList;

    pstList = ListRule_GetListByID(hCtx, uiListID);
    if (NULL == pstList)
    {
        return BS_NOT_FOUND;
    }

    return RuleList_IncreaseID(&pstList->stRuleList, uiStart, uiEnd, uiStep);
}

/* 移动rule */
BS_STATUS ListRule_MoveRule(
    IN LIST_RULE_HANDLE hCtx,
    IN UINT uiListID,
    IN UINT uiOldRuleID,
    IN UINT uiNewRuleID)
{
    LIST_RULE_LIST_S *pstList;

    pstList = ListRule_GetListByID(hCtx, uiListID);
    if (NULL == pstList)
    {
        return BS_NOT_FOUND;
    }

    return RuleList_MoveRule(&pstList->stRuleList, uiOldRuleID, uiNewRuleID);
}

RULE_NODE_S *ListRule_GetNextRule(
    IN LIST_RULE_HANDLE hCtx,
    IN UINT uiListID,
    IN UINT uiCurrentRuleID /* RULE_ID_INVALID 表示从头开始 */
)
{
    LIST_RULE_LIST_S *pstList;

    pstList = ListRule_GetListByID(hCtx, uiListID);
    if (NULL == pstList)
    {
        return NULL;
    }

    return RuleList_GetNextByID(&pstList->stRuleList, uiCurrentRuleID);
}

BS_STATUS ListRule_ResetID(IN LIST_RULE_HANDLE hCtx, IN UINT uiListID, IN UINT uiStep)
{
    LIST_RULE_LIST_S *pstList;

    pstList = ListRule_GetListByID(hCtx, uiListID);
    if (NULL == pstList)
    {
        return BS_NOT_FOUND;
    }

    return RuleList_ResetID(&pstList->stRuleList, uiStep);
}
