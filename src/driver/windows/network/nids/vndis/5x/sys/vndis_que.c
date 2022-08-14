/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2010-5-20
* Description:  HANDLE Stack. Stack中存放的是Handle
* History:     
******************************************************************************/
#include <ndis.h>

#include "vndis_def.h"
#include "vndis_que.h"
#include "vndis_mem.h"


#define VNDIS_CYCLE_INDEX_ADD(uiIndex, uiInc, uiMaxCount)   \
    {    \
        (uiIndex) += (uiInc); \
        if ((uiIndex) >= (uiMaxCount))     \
        {   \
            (uiIndex) -= (uiMaxCount); \
        }   \
    }

typedef struct
{
  UINT uiBase;
  UINT uiSize;
  UINT uiCapacity;
  VOID *pData[1];
}VNDIS_QUE_CTRL_S;

VNDIS_QUE_HANDLE VNDIS_QUE_Create(IN UINT uiCapacity)
{
    VNDIS_QUE_CTRL_S *pstQue;

    if (uiCapacity == 0)
    {
        return NULL;
    }

    pstQue = VNDIS_MEM_Malloc(sizeof(VNDIS_QUE_CTRL_S) + sizeof(VOID *) * (uiCapacity - 1));
    if (NULL == pstQue)
    {
        return NULL;
    }
    NdisZeroMemory(pstQue, sizeof(VNDIS_QUE_CTRL_S));
    pstQue->uiCapacity = uiCapacity;

    return pstQue;
}

VOID VNDIS_QUE_Destory(IN VNDIS_QUE_HANDLE hQue)
{
    VNDIS_QUE_CTRL_S *pstQue = hQue;

    if (pstQue != NULL)
    {
        VNDIS_MEM_Free(hQue, sizeof(VNDIS_QUE_CTRL_S) + sizeof(VOID *) * (pstQue->uiCapacity - 1));
    }
}

UINT VNDIS_QUE_Count(IN VNDIS_QUE_HANDLE hQue)
{
    VNDIS_QUE_CTRL_S *pstQue = hQue;

    if (pstQue == NULL)
    {
        return 0;
    }

    return pstQue->uiSize;
}

BOOLEAN VNDIS_QUE_IsFull(IN VNDIS_QUE_HANDLE hQue)
{
    VNDIS_QUE_CTRL_S *pstQue = hQue;

    if (pstQue == NULL)
    {
        return TRUE;
    }

    if (pstQue->uiSize == pstQue->uiCapacity)
    {
        return TRUE;
    }

    return FALSE;
}

VOID * VNDIS_QUE_Push(IN VNDIS_QUE_HANDLE hQue, IN VOID *pItem)
{
    UINT uiDest;
    VNDIS_QUE_CTRL_S *pstQue = hQue;
    VOID *pItemPushed = NULL;

    DEBUG_IN_FUNC();

    if ((pstQue != NULL) && (pstQue->uiSize < pstQue->uiCapacity))
    {
        uiDest = pstQue->uiBase;
        VNDIS_CYCLE_INDEX_ADD(uiDest, pstQue->uiSize, pstQue->uiCapacity);

        pstQue->pData[uiDest] = pItem;
        pstQue->uiSize ++;

        pItemPushed = pItem;
    }

    DEBUG_OUT_FUNC(pItemPushed);

    return pItemPushed;
}

VOID * VNDIS_QUE_Pop(IN VNDIS_QUE_HANDLE hQue)
{
    VNDIS_QUE_CTRL_S *pstQue = hQue;
    UINT uiSrc;
    VOID *pItem = NULL;

    DEBUG_IN_FUNC();

    if ((pstQue != NULL) && (pstQue->uiSize != 0))
    {
        uiSrc = pstQue->uiBase;
        VNDIS_CYCLE_INDEX_ADD(pstQue->uiBase, 1, pstQue->uiCapacity);
        pstQue->uiSize --;

        pItem = pstQue->pData[uiSrc];
    }

    DEBUG_OUT_FUNC(pItem);

    return pItem;
}

VOID * VNDIS_QUE_ExtractPop(IN VNDIS_QUE_HANDLE hQue, IN VOID *pItem)
{
    UINT uiDest, uiSrc, uiCount;
    VNDIS_QUE_CTRL_S *pstQue = hQue;
    VOID *pFound = NULL;

    DEBUG_IN_FUNC();

    if (pstQue != NULL)
    {
        uiDest = uiSrc = pstQue->uiBase;
        uiCount = pstQue->uiSize;

        while (uiCount--)
        {
            if (pItem == pstQue->pData[uiSrc])
            {
                pFound = pItem;
                pstQue->uiSize --;
            }
            else
            {
                pstQue->pData[uiDest] = pstQue->pData[uiSrc];
                VNDIS_CYCLE_INDEX_ADD(uiDest, 1, pstQue->uiCapacity);
            }

            VNDIS_CYCLE_INDEX_ADD(uiSrc, 1, pstQue->uiCapacity)
        }
    }

    DEBUG_OUT_FUNC(pFound);

    return pFound;
}


