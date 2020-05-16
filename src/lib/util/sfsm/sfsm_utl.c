/******************************************************************************
* Copyright (C),    LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2014-7-15
* Description: Simple fsm
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/sfsm_utl.h"

static inline VOID sfsm_Call(IN const SFSM_EVENT_S *pstEvent, IN UINT uiEvent, IN VOID *pUserContext)
{
    if (NULL != pstEvent->pfEventHandler)
    {
        pstEvent->pfEventHandler(uiEvent, pUserContext);
    }
}

VOID SFSM_EventHandle(IN SFSM_S *pstFsm, IN UINT uiEvent, IN VOID *pUserContext)
{
    UINT uiIndex;
    SFSM_EVENT_S *pstEvent;
    SFSM_EVENT_S *pstEventFound = NULL;

    if (NULL == pstFsm)
    {
        return;
    }

    for (uiIndex = 0; uiIndex < pstFsm->uiEventTblSize; uiIndex++)
    {
        pstEvent = &pstFsm->pstEventTbl[uiIndex];

        if ((pstFsm->uiState == pstEvent->uiState) && (uiEvent == pstEvent->uiEvent))
        {
            pstEventFound = pstEvent;
            break;
        }

        if ((pstEvent->uiState == SFSM_STATE_ANY) && (uiEvent == pstEvent->uiEvent))
        {
            pstEventFound = pstEvent;
        }
    }

    if (NULL != pstEventFound)
    {
        sfsm_Call(pstEventFound, uiEvent, pUserContext);
    }
}



