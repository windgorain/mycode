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

static void _listrule_DelListNode(LIST_RULE_CTRL_S *pstCtx, LIST_RULE_HEAD_S *pstListHead, PF_RULE_FREE pfFunc, void *ud)
{
    LIST_RULE_LIST_S *pstList = pstListHead->pstListRule;

    if (NULL != pstList) {
        ListRule_DestroyList((LIST_RULE_HANDLE)pstCtx, pstList, pfFunc, ud);
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

static LIST_RULE_HEAD_S *_listrule_GetListHeadByName(IN LIST_RULE_HANDLE ctx, IN CHAR *list_name)
{
    NAP_HANDLE hListNap = ctx->hListNap;
    LIST_RULE_HEAD_S *pstListHead;
    UINT uiIndex = NAP_INVALID_INDEX;

    while ((uiIndex = NAP_GetNextIndex(hListNap, uiIndex)) != NAP_INVALID_INDEX) {
        pstListHead = NAP_GetNodeByIndex(hListNap, uiIndex);
        if ((NULL != pstListHead->pstListRule) && (strcmp(pstListHead->pstListRule->list_name, list_name) == 0)) {
            return pstListHead;
        }
    }

    return NULL;
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

void ListRule_Destroy(LIST_RULE_HANDLE ctx, PF_RULE_FREE pfFunc, void *pUserHandle)
{
    ListRule_Finit(ctx, pfFunc, pUserHandle);
    MemCap_Free(ctx->memcap, ctx);
    return;
}

void ListRule_Reset(IN LIST_RULE_HANDLE ctx, IN PF_RULE_FREE pfFunc, IN VOID *pUserHandle)
{
    _listrule_reset(ctx, pfFunc, pUserHandle);
    return;
}


LIST_RULE_LIST_S * ListRule_CreateList(LIST_RULE_HANDLE ctx, char *list_name, int user_data_size)
{
    LIST_RULE_LIST_S *pstList;

    BS_DBGASSERT(strlen(list_name) < LIST_RULE_LIST_NAME_SIZE);

    pstList = MemCap_ZMalloc(ctx->memcap, sizeof(LIST_RULE_LIST_S) + user_data_size);
    if (! pstList) {
        return NULL;
    }

    RuleList_Init(&pstList->stRuleList);
    strlcpy(pstList->list_name, list_name, sizeof(pstList->list_name));

    pstList->default_action = BS_ACTION_UNDEF;

    return pstList;
}

VOID ListRule_DestroyList(IN LIST_RULE_HANDLE ctx, LIST_RULE_LIST_S *pstList, IN PF_RULE_FREE pfFunc, IN VOID *uh)
{
    if (NULL != pstList) {
        RuleList_Finit(&pstList->stRuleList, pfFunc, uh);
        MemCap_Free(ctx->memcap, pstList);
    }
}


UINT ListRule_AttachList(LIST_RULE_HANDLE ctx, LIST_RULE_LIST_S *list)
{
    LIST_RULE_HEAD_S *pstListHead;

    pstListHead = NAP_ZAlloc(ctx->hListNap);
    if (! pstListHead) {
        return 0;
    }

    pstListHead->pstListRule = list;

    list->list_id = NAP_GetIDByNode(ctx->hListNap, pstListHead);

    return list->list_id;
}


LIST_RULE_LIST_S * ListRule_DetachList(LIST_RULE_HANDLE ctx, UINT list_id)
{
    LIST_RULE_HEAD_S *pstListHead;

    pstListHead = NAP_GetNodeByID(ctx->hListNap, list_id);
    if (! pstListHead) {
        return NULL;
    }

    LIST_RULE_LIST_S *list = pstListHead->pstListRule;
    pstListHead->pstListRule = NULL;
    NAP_Free(ctx->hListNap, pstListHead);

    list->list_id = NAP_INVALID_ID;

    return list;
}


UINT ListRule_AddList(IN LIST_RULE_HANDLE ctx, char *list_name, int user_data_size)
{
    LIST_RULE_LIST_S *pstList;

    pstList = ListRule_CreateList(ctx, list_name, user_data_size);
    if (NULL == pstList) {
        return NAP_INVALID_ID;
    }

    UINT list_id = ListRule_AttachList(ctx, pstList);
    if (list_id == 0) {
        ListRule_DestroyList(ctx, pstList, NULL, NULL);
        return NAP_INVALID_ID;
    }

    return list_id;
}

void ListRule_DelList(LIST_RULE_HANDLE ctx, LIST_RULE_LIST_S *list, PF_RULE_FREE pfFunc, void *ud)
{
    ListRule_DelListID(ctx, list->list_id, pfFunc, ud);
}

void ListRule_DelListID(LIST_RULE_HANDLE ctx, UINT uiListID, PF_RULE_FREE pfFunc, void *ud)
{
    LIST_RULE_HEAD_S *pstListHead;

    pstListHead = NAP_GetNodeByID(ctx->hListNap, uiListID);
    if (NULL == pstListHead) {
        return;
    }

    _listrule_DelListNode(ctx, pstListHead, pfFunc, ud);
}

LIST_RULE_LIST_S *ListRule_GetListByID(IN LIST_RULE_HANDLE ctx, IN UINT uiListID)
{
    LIST_RULE_HEAD_S *pstListHead;

    pstListHead = NAP_GetNodeByID(ctx->hListNap, uiListID);
    if (NULL == pstListHead) {
        return NULL;
    }

    return pstListHead->pstListRule;
}


int ListRule_ReplaceList(LIST_RULE_HANDLE ctx, UINT uiListID,
        PF_RULE_FREE pfFunc, void *ud, LIST_RULE_LIST_S *pstListNew)
{
    LIST_RULE_HEAD_S *pstListHead;
    LIST_RULE_LIST_S *pstListOld;

    pstListHead = NAP_GetNodeByID(ctx->hListNap, uiListID);
    if (! pstListHead) {
        RETURN(BS_NOT_FOUND);
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
        ListRule_DestroyList(ctx, pstListOld, pfFunc, ud);
    }

    return BS_OK;
}


BS_STATUS ListRule_IncListRef(IN LIST_RULE_HANDLE ctx, IN UINT uiListID)
{
    LIST_RULE_LIST_S *pstList;

    pstList = ListRule_GetListByID(ctx, uiListID);
    if (NULL == pstList) {
        return BS_NOT_FOUND;
    }

    pstList->uiRefCount++;

    return BS_OK;
}


BS_STATUS ListRule_DecListRef(IN LIST_RULE_HANDLE ctx, IN UINT uiListID)
{
    LIST_RULE_LIST_S *pstList;

    pstList = ListRule_GetListByID(ctx, uiListID);
    if (NULL == pstList) {
        return BS_NOT_FOUND;
    }

    pstList->uiRefCount--;

    return BS_OK;
}


UINT ListRule_GetListRef(IN LIST_RULE_HANDLE ctx, IN UINT uiListID)
{
    LIST_RULE_LIST_S *pstList;

    pstList = ListRule_GetListByID(ctx, uiListID);
    if (NULL == pstList) {
        return 0;
    }

    return pstList->uiRefCount;
}


BOOL_T ListRule_IsAnyListRefed(IN LIST_RULE_HANDLE ctx)
{
    LIST_RULE_HEAD_S *head = NULL;

    while ((head = ListRule_GetNextList(ctx, head))) {
        if (head->pstListRule->uiRefCount > 0){
            return BOOL_TRUE;
        }
    }

    return BOOL_FALSE;
}

BS_ACTION_E ListRule_GetDefaultActionByID(IN LIST_RULE_HANDLE ctx, IN UINT uiListID)
{
    LIST_RULE_LIST_S *pstList;

    pstList = ListRule_GetListByID(ctx, uiListID);
    if (NULL == pstList) {
        return BS_ACTION_UNDEF;
    }

    return pstList->default_action;
}

BS_STATUS ListRule_SetDefaultActionByID(IN LIST_RULE_HANDLE ctx, IN UINT uiListID, BS_ACTION_E enAciton)
{
    LIST_RULE_LIST_S *pstList;

    pstList = ListRule_GetListByID(ctx, uiListID);
    if (NULL == pstList) {
        return BS_NO_SUCH;
    }

    pstList->default_action = enAciton;
    return BS_OK;
}

UINT ListRule_GetListIDByName(IN LIST_RULE_HANDLE ctx, IN CHAR *list_name)
{
    LIST_RULE_HEAD_S *pstListHead;

    pstListHead = _listrule_GetListHeadByName(ctx, list_name);
    if (NULL != pstListHead) {
        return NAP_GetIDByNode(ctx->hListNap, pstListHead);
    }

    return 0;
}

LIST_RULE_LIST_S * ListRule_GetListByName(IN LIST_RULE_HANDLE ctx, IN CHAR *list_name)
{
    LIST_RULE_HEAD_S *pstListHead;

    pstListHead = _listrule_GetListHeadByName(ctx, list_name);
    if (NULL != pstListHead) {
        return pstListHead->pstListRule;
    }

    return NULL;
}

LIST_RULE_HEAD_S * ListRule_GetNextList(IN LIST_RULE_HANDLE ctx, IN LIST_RULE_HEAD_S *curr)
{
    UINT index = NAP_INVALID_INDEX;

    if (curr) {
        index = NAP_GetIndexByNode(ctx->hListNap, curr);
        if (index == NAP_INVALID_INDEX) {
            return NULL;
        }
    }

    index = NAP_GetNextIndex(ctx->hListNap, index);
    if (index == NAP_INVALID_INDEX) {
        return NULL;
    }

    return NAP_GetNodeByIndex(ctx->hListNap, index);
}

UINT ListRule_GetNextListID(IN LIST_RULE_HANDLE ctx, IN UINT ulCurrentListID )
{
    return NAP_GetNextID(ctx->hListNap, ulCurrentListID);
}

CHAR * ListRule_GetListNameByID(IN LIST_RULE_HANDLE ctx, IN UINT ulListID)
{
    LIST_RULE_LIST_S *pstList;
    pstList = ListRule_GetListByID(ctx, ulListID);
    if (NULL == pstList) {
        return NULL;
    }
    return pstList->list_name;
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

int ListRule_AddRule(IN LIST_RULE_HANDLE hCtx, IN UINT uiListID, IN UINT uiRuleID, IN RULE_NODE_S *pstRule)
{
    LIST_RULE_LIST_S *pstList;

    pstList = ListRule_GetListByID(hCtx, uiListID);
    if (NULL == pstList)
    {
        return BS_NO_SUCH;
    }

    return ListRule_AddRule2List(pstList, uiRuleID, pstRule);
}


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


VOID ListRule_ResetRuleID(IN LIST_RULE_HANDLE hCtx, IN UINT uiListID, IN UINT step )
{
    LIST_RULE_LIST_S *pstList;

    if (step == 0) {
        step = 1;
    }

    pstList = ListRule_GetListByID(hCtx, uiListID);
    if (NULL == pstList) {
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


RULE_NODE_S *ListRule_GetNextRule(LIST_RULE_HANDLE hCtx, UINT uiListID, UINT uiCurrentRuleID)
{
    LIST_RULE_LIST_S *pstList;

    pstList = ListRule_GetListByID(hCtx, uiListID);
    if (! pstList) {
        return NULL;
    }

    return RuleList_GetNextByID(&pstList->stRuleList, uiCurrentRuleID);
}

BS_STATUS ListRule_ResetID(IN LIST_RULE_HANDLE hCtx, IN UINT uiListID, IN UINT uiStep)
{
    LIST_RULE_LIST_S *pstList;

    pstList = ListRule_GetListByID(hCtx, uiListID);
    if (! pstList) {
        return BS_NOT_FOUND;
    }

    return RuleList_ResetID(&pstList->stRuleList, uiStep);
}

void ListRule_WalkList(LIST_RULE_HANDLE hCtx, PF_LIST_RULE_WALK_LIST_FUNC walk_list, void *ud)
{
    LIST_RULE_HEAD_S *head = NULL;

    while ((head = ListRule_GetNextList(hCtx, head))) {
        walk_list(head->pstListRule, ud);
    }
}

void ListRule_WalkRule(LIST_RULE_LIST_S *list, PF_LIST_RULE_WALK_RULE_FUNC walk_rule, void *ud)
{
    RULE_NODE_S *rule = NULL;

    while ((rule = RuleList_GetNextByNode(&list->stRuleList, rule))) {
        walk_rule(list, rule, ud);
    }
}

void ListRule_Walk(LIST_RULE_HANDLE hCtx, PF_LIST_RULE_WALK_LIST_FUNC walk_list,
        PF_LIST_RULE_WALK_RULE_FUNC walk_rule, void *ud)
{
    LIST_RULE_HEAD_S *head = NULL;
    RULE_NODE_S *rule = NULL;

    while ((head = ListRule_GetNextList(hCtx, head))) {

        if (walk_list) {
            walk_list(head->pstListRule, ud);
        }

        if (! walk_rule) {
            continue;
        }

        while ((rule = RuleList_GetNextByNode(&head->pstListRule->stRuleList, rule))) {
            walk_rule(head->pstListRule, rule, ud);
        }
    }
}


