/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-5-22
* Description: 有限状态机
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/lstr_utl.h"
#include "utl/mempool_utl.h"
#include "utl/fsm_utl.h"

typedef struct
{
    DLL_NODE_S stLink;

    UINT uiEvent;
    UINT uiNextState;
    PF_FSM_EVENT_FUNC pfActionFunc;
}_FSM_SWITCH_EVENT_S;

typedef struct
{
    UINT uiState;
    CHAR *pcState;
    DLL_HEAD_S stEventList;
}_FSM_SWITCH_LIST_S;

typedef struct
{
    DLL_NODE_S stLink;
    PF_FSM_STATE_LISTEN pfFunc;
    USER_HANDLE_S stUserHandle;
}_FSM_STATE_LISTENER_S;

typedef struct
{
    
    FSM_STATE_MAP_S *pstStateMap;
    UINT uiStateMapCount;
    FSM_EVENT_MAP_S *pstEventMap;
    UINT uiEventMapCount;
    FSM_SWITCH_MAP_S *pstSwitchMap;   
    UINT uiSwitchMapCount;

    
    MEMPOOL_HANDLE hMemPool;

    
    _FSM_SWITCH_LIST_S *pstSwitchList;  
    UINT uiSwitchTblCount;

    
    DLL_HEAD_S stStateListenerList;   
}_FSM_SWITCH_TBL_S;

static UINT fsm_GetStateByName(IN _FSM_SWITCH_TBL_S *pstSwitchTbl, IN CHAR *pcStateName, IN UINT uiStateNameLen)
{
    UINT i;
    UINT uiStateCount = pstSwitchTbl->uiStateMapCount;
    FSM_STATE_MAP_S *pstStateMap = pstSwitchTbl->pstStateMap;

    if (strncmp(FSM_STATE_NO_CHANGE_STRING, pcStateName, uiStateNameLen) == 0)
    {
        return FSM_STATE_NO_CHANGE;
    }

    if (strncmp(FSM_STATE_ANY_STRING, pcStateName, uiStateNameLen) == 0)
    {
        return FSM_STATE_ANY;
    }

    for (i=0; i<uiStateCount; i++)
    {
        if (strlen(pstStateMap[i].pcStateName) != uiStateNameLen)
        {
            continue;
        }

        if (strncmp(pstStateMap[i].pcStateName, pcStateName, uiStateNameLen) == 0)
        {
            return pstStateMap[i].uiState;
        }
    }

    BS_DBGASSERT(0);

    return FSM_INVALID_STATE;
}

static UINT fsm_GetEventByName(IN _FSM_SWITCH_TBL_S *pstSwitchTbl, IN CHAR *pcEventName, IN UINT uiEventNameLen)
{
    UINT i;
    UINT uiCount = pstSwitchTbl->uiEventMapCount;
    FSM_EVENT_MAP_S *pstMap = pstSwitchTbl->pstEventMap;

    for (i=0; i<uiCount; i++)
    {
        if (strlen(pstMap[i].pcEventName) != uiEventNameLen)
        {
            continue;
        }

        if (strncmp(pstMap[i].pcEventName, pcEventName, uiEventNameLen) == 0)
        {
            return pstMap[i].uiEvent;
        }
    }

    BS_DBGASSERT(0);

    return FSM_INVALID_EVENT;
}

CHAR * FSM_GetStateName(IN FSM_SWITCH_TBL hSwitchTbl, IN UINT uiState)
{
    UINT i;
    _FSM_SWITCH_TBL_S *pstSwitchTbl = hSwitchTbl;
    FSM_STATE_MAP_S *pstStateMap = pstSwitchTbl->pstStateMap;

    for (i=0; i<pstSwitchTbl->uiSwitchTblCount; i++)
    {
        if (pstStateMap[i].uiState == uiState)
        {
            return pstStateMap[i].pcStateName;
        }
    }

    BS_DBGASSERT(0);

    return "";
}

CHAR * FSM_GetEventName(IN FSM_SWITCH_TBL hSwitchTbl, IN UINT uiEvent)
{
    UINT i;
    _FSM_SWITCH_TBL_S *pstSwitchTbl = hSwitchTbl;
    UINT uiCount = pstSwitchTbl->uiEventMapCount;
    FSM_EVENT_MAP_S *pstMap = pstSwitchTbl->pstEventMap;

    for (i=0; i<uiCount; i++)
    {
        if (pstMap[i].uiEvent == uiEvent)
        {
            return pstMap[i].pcEventName;
        }
    }

    BS_DBGASSERT(0);

    return "";
}

static _FSM_SWITCH_LIST_S * fsm_GetSwitchList(IN _FSM_SWITCH_TBL_S *pstSwitchTbl, IN UINT uiState)
{
    UINT i;

    for (i=0; i<pstSwitchTbl->uiSwitchTblCount; i++)
    {
        if (pstSwitchTbl->pstSwitchList[i].uiState == uiState)
        {
            return &pstSwitchTbl->pstSwitchList[i];
        }
    }

    return NULL;
}

static BS_STATUS fsm_AddEventNode(IN _FSM_SWITCH_TBL_S *pstSwitchTbl, IN UINT uiState, IN UINT uiEvent, IN FSM_SWITCH_MAP_S *pstSwitchMapLine)
{
    UINT uiNextState;
    _FSM_SWITCH_EVENT_S *pstEventNode;
    _FSM_SWITCH_LIST_S *pstSwitchList;
    UINT uiNextStateLen;

    uiNextStateLen = strlen(pstSwitchMapLine->pcNextState);

    uiNextState = fsm_GetStateByName(pstSwitchTbl, pstSwitchMapLine->pcNextState, uiNextStateLen);
    if (uiNextState == FSM_INVALID_STATE)
    {
        BS_WARNNING(("Can' find next state:%s.", pstSwitchMapLine->pcNextState));
        return BS_ERR;
    }

    pstSwitchList = fsm_GetSwitchList(pstSwitchTbl, uiState);
    if (NULL == pstSwitchList)
    {
        return BS_ERR;
    }

    pstEventNode = MEMPOOL_ZAlloc(pstSwitchTbl->hMemPool, sizeof(_FSM_SWITCH_EVENT_S));
    if (NULL == pstEventNode)
    {
        return BS_ERR;
    }

    pstEventNode->pfActionFunc = pstSwitchMapLine->pfEventFunc;
    pstEventNode->uiEvent = uiEvent;
    pstEventNode->uiNextState = uiNextState;

    DLL_ADD(&pstSwitchList->stEventList, pstEventNode);

    return BS_OK;
}

static BS_STATUS fsm_ParseSwitchEvent(IN _FSM_SWITCH_TBL_S *pstSwitchTbl, IN UINT uiState, IN FSM_SWITCH_MAP_S *pstSwitchMapLine)
{
    LSTR_S stString;
    UINT uiEvent;

    LSTR_SCAN_ELEMENT_BEGIN(pstSwitchMapLine->pcEvent, strlen(pstSwitchMapLine->pcEvent), ',', &stString)
    {
        uiEvent = fsm_GetEventByName(pstSwitchTbl, stString.pcData, stString.uiLen);
        if (uiEvent == FSM_INVALID_EVENT)
        {
            BS_WARNNING(("Can' find event:%s.", stString.pcData));
            return BS_ERR;
        }

        if (BS_OK != fsm_AddEventNode(pstSwitchTbl, uiState, uiEvent, pstSwitchMapLine))
        {
            return BS_ERR;
        }
    }LSTR_SCAN_ELEMENT_END();

    return BS_OK;
}

static BS_STATUS fsm_ParseSwichMapLine(IN _FSM_SWITCH_TBL_S *pstSwitchTbl, IN FSM_SWITCH_MAP_S *pstSwitchMapLine)
{
    LSTR_S stString;
    UINT uiState;

    LSTR_SCAN_ELEMENT_BEGIN(pstSwitchMapLine->pcState, strlen(pstSwitchMapLine->pcState), ',', &stString)
    {
        uiState = fsm_GetStateByName(pstSwitchTbl, stString.pcData, stString.uiLen);
        if (uiState == FSM_INVALID_STATE)
        {
            BS_WARNNING(("Can' find state:%s.", stString.pcData));
            return BS_ERR;
        }

        if (BS_OK != fsm_ParseSwitchEvent(pstSwitchTbl, uiState, pstSwitchMapLine))
        {
            return BS_ERR;
        }
    }LSTR_SCAN_ELEMENT_END();

    return BS_OK;
}

static BS_STATUS fsm_ParseSwitchMap(IN _FSM_SWITCH_TBL_S *pstSwitchTbl)
{
    UINT i;

    for (i=0; i<pstSwitchTbl->uiSwitchMapCount; i++)
    {
        if (BS_OK != fsm_ParseSwichMapLine(pstSwitchTbl, &pstSwitchTbl->pstSwitchMap[i]))
        {
            return BS_ERR;
        }
    }

    return BS_OK;
}

static BS_STATUS fsm_CreateStateList(IN _FSM_SWITCH_TBL_S *pstSwitchTbl)
{
    _FSM_SWITCH_LIST_S *pstSwitchList;
    UINT uiStateCount;
    UINT i;

    uiStateCount = pstSwitchTbl->uiStateMapCount;
    
    pstSwitchList = MEMPOOL_ZAlloc(pstSwitchTbl->hMemPool, sizeof(_FSM_SWITCH_LIST_S) * (uiStateCount + 2));
    if (NULL == pstSwitchList)
    {
        return BS_NO_MEMORY;
    }

    for (i=0; i<uiStateCount; i++)
    {
        pstSwitchList[i].pcState = pstSwitchTbl->pstStateMap[i].pcStateName;
        pstSwitchList[i].uiState = pstSwitchTbl->pstStateMap[i].uiState;
        DLL_INIT(&pstSwitchList[i].stEventList);
    }

    pstSwitchList[uiStateCount].pcState = "*";
    pstSwitchList[uiStateCount].uiState = FSM_STATE_ANY;
    DLL_INIT(&pstSwitchList[uiStateCount].stEventList);

    pstSwitchList[uiStateCount + 1].pcState = "@";
    pstSwitchList[uiStateCount + 1].uiState = FSM_STATE_NO_CHANGE;
    DLL_INIT(&pstSwitchList[uiStateCount + 1].stEventList);

    pstSwitchTbl->uiSwitchTblCount = uiStateCount + 2;
    pstSwitchTbl->pstSwitchList = pstSwitchList;

    return BS_OK;
}

static BS_STATUS fsm_BuildSwitchList(IN _FSM_SWITCH_TBL_S *pstSwitchTbl)
{
    if (fsm_CreateStateList(pstSwitchTbl))
    {
        return BS_ERR;
    }

    return fsm_ParseSwitchMap(pstSwitchTbl);
}

static VOID fsm_NotifyStateListener(IN FSM_S *pstFsm)
{
    _FSM_STATE_LISTENER_S *pstListener;
    _FSM_SWITCH_TBL_S *pstSwitchTbl = pstFsm->hSwitchTbl;

    DLL_SCAN(&pstSwitchTbl->stStateListenerList, pstListener)
    {
        pstListener->pfFunc(pstFsm, pstFsm->uiOldState, pstFsm->uiCurState, &pstListener->stUserHandle);
    }
}

FSM_SWITCH_TBL FSM_CreateSwitchTbl
(
    IN FSM_STATE_MAP_S *pstStateMap,
    IN UINT uiStateMapCount,
    IN FSM_EVENT_MAP_S *pstEventMap,
    IN UINT uiEventMapCount,
    IN FSM_SWITCH_MAP_S *pstSwitchMap,
    IN UINT uiSwitchMapCount
)
{
    _FSM_SWITCH_TBL_S *pstSwitchTbl;

    if ((NULL == pstStateMap) || (NULL == pstEventMap) || (NULL == pstSwitchMap))
    {
        return NULL;
    }

    pstSwitchTbl = MEM_ZMalloc(sizeof(_FSM_SWITCH_TBL_S));
    if (NULL == pstSwitchTbl)
    {
        return NULL;
    }

    pstSwitchTbl->hMemPool = MEMPOOL_Create(0);
    if (NULL == pstSwitchTbl->hMemPool)
    {
        MEM_Free(pstSwitchTbl);
        return NULL;
    }

    pstSwitchTbl->pstStateMap = pstStateMap;
    pstSwitchTbl->uiStateMapCount = uiStateMapCount;
    pstSwitchTbl->pstEventMap = pstEventMap;
    pstSwitchTbl->uiEventMapCount = uiEventMapCount;
    pstSwitchTbl->pstSwitchMap = pstSwitchMap;
    pstSwitchTbl->uiSwitchMapCount = uiSwitchMapCount;
    DLL_INIT(&pstSwitchTbl->stStateListenerList);

    if (BS_OK != fsm_BuildSwitchList(pstSwitchTbl))
    {
        FSM_DestorySwitchTbl(pstSwitchTbl);
        return NULL;
    }

    return pstSwitchTbl;
}

VOID FSM_DestorySwitchTbl(IN FSM_SWITCH_TBL hSwitchTbl)
{
    _FSM_SWITCH_TBL_S *pstSwitchTbl = hSwitchTbl;

    if (NULL == pstSwitchTbl)
    {
        return;
    }

    MEMPOOL_Destory(pstSwitchTbl->hMemPool);

    MEM_Free(pstSwitchTbl);
}

VOID FSM_Init(INOUT FSM_S *pstFsm, IN FSM_SWITCH_TBL hSwitchTbl)
{
    Mem_Zero(pstFsm, sizeof(FSM_S));

    pstFsm->hSwitchTbl = hSwitchTbl;
}

VOID FSM_Finit(IN FSM_S *pstFsm)
{
    if (pstFsm->hEventQue != NULL)
    {
        QUE_Destory(pstFsm->hEventQue);
    }
}

BS_STATUS FSM_InitEventQue(IN FSM_S *pstFsm, IN UINT uiCapacity)
{
    pstFsm->hEventQue = QUE_Create(uiCapacity, 0);

    if (NULL == pstFsm->hEventQue)
    {
        return BS_NO_MEMORY;
    }

    return BS_OK;
}

VOID FSM_InitState(IN FSM_S *pstFsm, IN UINT uiInitState)
{
    pstFsm->uiCurState = uiInitState;
    BS_DBG_OUTPUT(pstFsm->uiDbgFlag, FSM_DBG_FLAG_STATE_CHANGE,
        ("FSM(%p): Init state to %s\r\n", pstFsm, FSM_GetStateName(pstFsm->hSwitchTbl, pstFsm->uiCurState)));
}

VOID FSM_SetState(IN FSM_S *pstFsm, IN UINT uiState)
{
    if (pstFsm->uiCurState == uiState)
    {
        return;
    }

    if (uiState == FSM_STATE_NO_CHANGE)
    {
        return;
    }

    pstFsm->uiOldState = pstFsm->uiCurState;
    pstFsm->uiCurState = uiState;

    BS_DBG_OUTPUT(pstFsm->uiDbgFlag, FSM_DBG_FLAG_STATE_CHANGE,
        ("FSM(%p): Change state from %s to %s\r\n", pstFsm,
        FSM_GetStateName(pstFsm->hSwitchTbl, pstFsm->uiOldState),FSM_GetStateName(pstFsm->hSwitchTbl, pstFsm->uiCurState)));

    fsm_NotifyStateListener(pstFsm);
}

static _FSM_SWITCH_EVENT_S * fsm_FindNomalEventAction(IN _FSM_SWITCH_TBL_S *pstSwitchTbl, IN UINT uiState, IN UINT uiEvent)
{
    _FSM_SWITCH_LIST_S *pstSwitchList;
    _FSM_SWITCH_EVENT_S *pstEvent;
    _FSM_SWITCH_EVENT_S *pstEventFound = NULL;

    pstSwitchList = fsm_GetSwitchList(pstSwitchTbl, uiState);
    if (NULL == pstSwitchList)
    {
        return NULL;
    }

    DLL_SCAN(&pstSwitchList->stEventList, pstEvent)
    {
        if (pstEvent->uiEvent == uiEvent)
        {
            pstEventFound = pstEvent;
            break;
        }
    }

    return pstEventFound;
}

static _FSM_SWITCH_EVENT_S * fsm_FindAnyStateEventAction(IN _FSM_SWITCH_TBL_S *pstSwitchTbl, IN UINT uiEvent)
{
    _FSM_SWITCH_LIST_S *pstSwitchList;
    _FSM_SWITCH_EVENT_S *pstEvent;
    _FSM_SWITCH_EVENT_S *pstEventFound = NULL;

    pstSwitchList = fsm_GetSwitchList(pstSwitchTbl, FSM_STATE_ANY);
    if (NULL == pstSwitchList)
    {
        return NULL;
    }

    DLL_SCAN(&pstSwitchList->stEventList, pstEvent)
    {
        if (pstEvent->uiEvent == uiEvent)
        {
            pstEventFound = pstEvent;
            break;
        }
    }

    return pstEventFound;
}

static _FSM_SWITCH_EVENT_S * fsm_FindEventAction(IN _FSM_SWITCH_TBL_S *pstSwitchTbl, IN UINT uiState, IN UINT uiEvent)
{
    _FSM_SWITCH_EVENT_S *pstEventAction;

    pstEventAction = fsm_FindNomalEventAction(pstSwitchTbl, uiState, uiEvent);
    if (NULL == pstEventAction)
    {
        pstEventAction = fsm_FindAnyStateEventAction(pstSwitchTbl, uiEvent);
    }

    return pstEventAction;
}

static BS_STATUS _fsm_EventHandle(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    _FSM_SWITCH_EVENT_S *pstEventFound = NULL;
    BS_STATUS eRet = BS_OK;

    BS_DBG_OUTPUT(pstFsm->uiDbgFlag, FSM_DBG_FLAG_EVENT,
        ("FSM(%p): Receive event %s.\r\n", pstFsm, FSM_GetEventName(pstFsm->hSwitchTbl, uiEvent)));

    pstEventFound = fsm_FindEventAction(pstFsm->hSwitchTbl, pstFsm->uiCurState, uiEvent);

    if (pstEventFound != NULL)
    {
        FSM_SetState(pstFsm, pstEventFound->uiNextState);
        if (NULL != pstEventFound->pfActionFunc)
        {
            eRet = pstEventFound->pfActionFunc(pstFsm, uiEvent);
        }
    }
    else
    {
        BS_DBG_OUTPUT(pstFsm->uiDbgFlag, FSM_DBG_FLAG_ERR,
            ("FSM(%p): Can't find event %s in state %s.\r\n",
            pstFsm,
            FSM_GetEventName(pstFsm->hSwitchTbl, uiEvent),
            FSM_GetStateName(pstFsm->hSwitchTbl, pstFsm->uiCurState)));
    }

    return eRet;
}


BS_STATUS FSM_PushEvent(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    BS_DBG_OUTPUT(pstFsm->uiDbgFlag, FSM_DBG_FLAG_EVENT,
            ("FSM(%p): Push event %s.\r\n", pstFsm, FSM_GetEventName(pstFsm->hSwitchTbl, uiEvent)));

    if (NULL == pstFsm->hEventQue)
    {
        BS_DBGASSERT(0);
        return BS_NOT_SUPPORT;
    }

    if (BS_OK != QUE_Push(pstFsm->hEventQue, UINT_HANDLE(uiEvent)))
    {
        return BS_ERR;
    }

    return BS_OK;
}

UINT FSM_PopEvent(IN FSM_S *pstFsm)
{
    HANDLE hEvent;

    if (NULL == pstFsm->hEventQue)
    {
        return FSM_INVALID_EVENT;
    }

    if (BS_OK != QUE_Pop(pstFsm->hEventQue, &hEvent))
    {
        return FSM_INVALID_EVENT;
    }

    BS_DBG_OUTPUT(pstFsm->uiDbgFlag, FSM_DBG_FLAG_EVENT,
            ("FSM(%p): Pop event %s.\r\n", pstFsm, FSM_GetEventName(pstFsm->hSwitchTbl, (UINT)(ULONG)hEvent)));

    return (UINT)(ULONG)hEvent;
}

BS_STATUS FSM_EventHandle(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    UINT uiEventTmp = uiEvent;
    BS_STATUS eRet = BS_OK;

    do {
        eRet = _fsm_EventHandle(pstFsm, uiEventTmp);
    }while ((eRet != BS_STOP) && ((uiEventTmp = FSM_PopEvent(pstFsm)) != FSM_INVALID_EVENT));

    return eRet;
}

BS_STATUS FSM_RegStateListener(IN FSM_SWITCH_TBL hSwitchTbl, IN PF_FSM_STATE_LISTEN pfListenFunc, IN USER_HANDLE_S *pstUserHandle)
{
    _FSM_STATE_LISTENER_S *pstListener;
    _FSM_SWITCH_TBL_S *pstSwitchTbl = hSwitchTbl;

    pstListener = MEMPOOL_ZAlloc(pstSwitchTbl->hMemPool, sizeof(_FSM_STATE_LISTENER_S));
    if (NULL == pstListener)
    {
        return BS_NO_MEMORY;
    }

    pstListener->pfFunc = pfListenFunc;
    if (NULL != pstUserHandle)
    {
        pstListener->stUserHandle = *pstUserHandle;
    }

    DLL_ADD(&pstSwitchTbl->stStateListenerList, pstListener);

    return BS_OK;
}


