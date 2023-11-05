/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2010-5-20
* Description:  队列. 队列中存放的是指针
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/que_utl.h"

typedef struct
{
  UINT uiBase;
  UINT uiCount;
  UINT uiCapacity;
  UINT uiFlag;
  HANDLE ahData[0];
}QUE_CTRL_S;


QUE_HANDLE QUE_Create(IN UINT uiCapacity, IN UINT uiFlag)
{
    QUE_CTRL_S *pstQue;

    pstQue = MEM_Malloc(sizeof(QUE_CTRL_S) + sizeof(HANDLE) * uiCapacity);
    if (NULL == pstQue)
    {
        return NULL;
    }
    Mem_Zero(pstQue, sizeof(QUE_CTRL_S));

    pstQue->uiCapacity = uiCapacity;
    pstQue->uiFlag = uiFlag;

    return pstQue;
}

VOID QUE_Destory(IN QUE_HANDLE hQue)
{
    if (hQue != NULL)
    {
        MEM_Free(hQue);
    }
}

UINT QUE_Count(IN QUE_HANDLE hQue)
{
    QUE_CTRL_S *pstQue = hQue;

    return pstQue->uiCount;
}

BOOL_T QUE_IsFull(IN QUE_HANDLE hQue)
{
    QUE_CTRL_S *pstQue = hQue;

    if (pstQue->uiCount == pstQue->uiCapacity)
    {
        return TRUE;
    }

    return FALSE;
}

BS_STATUS QUE_Push(IN QUE_HANDLE hQue, IN HANDLE hItem)
{
    UINT uiDest;
    HANDLE hHandle;
    QUE_CTRL_S *pstQue = hQue;

    if (pstQue->uiCount >= pstQue->uiCapacity)
    {
        if (pstQue->uiFlag & QUE_FLAG_OVERWRITE)
        {
            QUE_Pop(hQue, &hHandle);
        }
        else
        {
            return BS_ERR;
        }
    }

    uiDest = pstQue->uiBase;
    NUM_CYCLE_INDEX_ADD(uiDest, pstQue->uiCount, pstQue->uiCapacity);

    pstQue->ahData[uiDest] = hItem;
    pstQue->uiCount ++;

    return BS_OK;
}

BS_STATUS QUE_Pop(IN QUE_HANDLE hQue, OUT HANDLE *phItem)
{
    QUE_CTRL_S *pstQue = hQue;
    UINT uiSrc;

    if (pstQue->uiCount == 0)
    {
        return BS_EMPTY;
    }

    uiSrc = pstQue->uiBase;
    NUM_CYCLE_INDEX_ADD(pstQue->uiBase, 1, pstQue->uiCapacity);
    pstQue->uiCount --;

    *phItem = pstQue->ahData[uiSrc];

    return BS_OK;
}


BS_STATUS QUE_ExtractPop(IN QUE_HANDLE hQue, IN HANDLE hItem, OUT HANDLE *phItemFound)
{
    UINT uiDest, uiSrc, uiCount;
    QUE_CTRL_S *pstQue = hQue;
    BS_STATUS eRet = BS_NOT_FOUND;

    uiDest = uiSrc = pstQue->uiBase;
    uiCount = pstQue->uiCount;

    while (uiCount--)
    {
        if (hItem == pstQue->ahData[uiSrc])
        {
            *phItemFound = hItem;
            pstQue->uiCount --;
            eRet = BS_OK;
        }
        else
        {
            pstQue->ahData[uiDest] = pstQue->ahData[uiSrc];
            NUM_CYCLE_INDEX_ADD(uiDest, 1, pstQue->uiCapacity);
        }

        NUM_CYCLE_INDEX_ADD(uiSrc, 1, pstQue->uiCapacity);
    }

    return eRet;
}

