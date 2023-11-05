/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-6-21
* Description: byte que, 字节队列,里面放的是一个个字节
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/que_utl.h"

typedef struct
{
  UINT uiCapacity;
  UINT uiBase;
  UINT uiCount;
  UINT uiFlag;
  UCHAR aucData[0];
}BYTE_QUE_CTRL_S;

BYTE_QUE_HANDLE ByteQue_Create(IN UINT uiCapacity, IN UINT uiFlag)
{
    BYTE_QUE_CTRL_S *pstQue;

    pstQue = MEM_Malloc(sizeof(BYTE_QUE_CTRL_S) + sizeof(UCHAR) * uiCapacity);
    if (NULL == pstQue)
    {
        return NULL;
    }
    Mem_Zero(pstQue, sizeof(BYTE_QUE_CTRL_S));

    pstQue->uiCapacity = uiCapacity;
    pstQue->uiFlag = uiFlag;

    return pstQue;
}

VOID ByteQue_Destory(IN BYTE_QUE_HANDLE hQue)
{
    if (hQue != NULL)
    {
        MEM_Free(hQue);
    }
}

VOID ByteQue_Clear(IN BYTE_QUE_HANDLE hQue)
{
    BYTE_QUE_CTRL_S *pstQue = hQue;

    pstQue->uiBase = 0;
    pstQue->uiCount = 0;
}

UINT ByteQue_Count(IN BYTE_QUE_HANDLE hQue)
{
    BYTE_QUE_CTRL_S *pstQue = hQue;

    return pstQue->uiCount;
}

BOOL_T ByteQue_IsFull(IN BYTE_QUE_HANDLE hQue)
{
    BYTE_QUE_CTRL_S *pstQue = hQue;

    if (pstQue->uiCount >= pstQue->uiCapacity)
    {
        return TRUE;
    }

    return FALSE;
}

BS_STATUS ByteQue_Push(IN BYTE_QUE_HANDLE hQue, IN UCHAR ucChar)
{
    UINT uiDest;
    BYTE_QUE_CTRL_S *pstQue = hQue;

    if (pstQue->uiCount >= pstQue->uiCapacity)
    {
        if (pstQue->uiFlag & QUE_FLAG_OVERWRITE)
        {
            ByteQue_Pop(hQue);
        }
        else
        {
            return BS_FULL;
        }
    }

    uiDest = pstQue->uiBase;
    NUM_CYCLE_INDEX_ADD(uiDest, pstQue->uiCount, pstQue->uiCapacity);

    pstQue->aucData[uiDest] = ucChar;
    pstQue->uiCount ++;

    return BS_OK;
}

UINT ByteQue_Pop(IN QUE_HANDLE hQue)
{
    BYTE_QUE_CTRL_S *pstQue = hQue;
    UINT uiSrc;

    if (pstQue->uiCount == 0)
    {
        return BYTE_QUE_POP_ERR;
    }

    uiSrc = pstQue->uiBase;
    NUM_CYCLE_INDEX_ADD(pstQue->uiBase, 1, pstQue->uiCapacity);
    pstQue->uiCount --;

    return pstQue->aucData[uiSrc];
}

UINT ByteQue_PopSpec(IN QUE_HANDLE hQue, IN UINT uiIndex)
{
    BYTE_QUE_CTRL_S *pstQue = hQue;
    UINT uiSrc;
    UINT uiDst;
    UCHAR ucChar;
    UINT uiLoop;

    if (uiIndex >= pstQue->uiCount)
    {
        return BYTE_QUE_POP_ERR;
    }

    uiDst = pstQue->uiBase;
    NUM_CYCLE_INDEX_ADD(uiDst, uiIndex, pstQue->uiCapacity);
    ucChar = pstQue->aucData[uiDst];

    for (uiLoop = uiIndex + 1; uiLoop < pstQue->uiCount; uiLoop++)
    {
        uiSrc = uiDst;
        NUM_CYCLE_INDEX_ADD(uiSrc, 1, pstQue->uiCapacity);
        pstQue->aucData[uiDst] = pstQue->aucData[uiSrc];
        NUM_CYCLE_INDEX_ADD(uiDst, 1, pstQue->uiCapacity);
    }

    pstQue->uiCount --;

    return ucChar;
}


UINT ByteQue_PopTail(IN QUE_HANDLE hQue)
{
    BYTE_QUE_CTRL_S *pstQue = hQue;

    if (pstQue->uiCount == 0)
    {
        return BYTE_QUE_POP_ERR;
    }

    return ByteQue_PopSpec(hQue, pstQue->uiCount - 1);
}

UINT ByteQue_PeekOne(IN IN QUE_HANDLE hQue, IN UINT uiIndex)
{
    BYTE_QUE_CTRL_S *pstQue = hQue;
    UINT uiSrc;

    if (uiIndex >= pstQue->uiCount)
    {
        return BYTE_QUE_POP_ERR;
    }

    uiSrc = pstQue->uiBase;
    NUM_CYCLE_INDEX_ADD(uiSrc, uiIndex, pstQue->uiCapacity);

    return pstQue->aucData[uiSrc];
}


UINT ByteQue_Peek
(
    IN IN QUE_HANDLE hQue, 
    IN UINT uiIndex,
    OUT UCHAR *pucBuf,
    IN UINT uiCount
)
{
    BYTE_QUE_CTRL_S *pstQue = hQue;
    UINT uiCopyCount;
    UINT uiSrc;
    UINT i;

    if (uiIndex >= pstQue->uiCount)
    {
        return 0;
    }

    uiCopyCount = MIN(uiCount, pstQue->uiCount - uiIndex);

    uiSrc = pstQue->uiBase;
    NUM_CYCLE_INDEX_ADD(uiSrc, uiIndex, pstQue->uiCapacity);

    for (i=0; i<uiCopyCount; i++)
    {
        pucBuf[i] = pstQue->aucData[uiSrc];
        NUM_CYCLE_INDEX_ADD(uiSrc, 1, pstQue->uiCapacity);
    }

    return uiCopyCount;
}

