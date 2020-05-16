/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-11-16
* Description: 动态增长的数组, 有效值不为0，如果将值设置为0则认为是空闲位置
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/darray_utl.h"

#define _DARRAY_DFT_STEP_SIZE 128

typedef struct
{
    UINT uiSize;  /* 数组大小 */
    UINT uiCount; /* 数组里面挂了多少个数据 */
    HANDLE * phArray;
}_DARRAY_CTRL_S;


STATIC BS_STATUS darray_Expand(IN _DARRAY_CTRL_S *pstCtrl, IN UINT uiNewSize)
{
    VOID *pMem;
    UINT uiOldSize;

    if (pstCtrl->uiSize >= uiNewSize)
    {
        return BS_OK;
    }

    uiOldSize = pstCtrl->uiSize;

    pMem = MEM_MallocAndCopy(pstCtrl->phArray, pstCtrl->uiSize * sizeof(HANDLE), uiNewSize * sizeof(HANDLE));
    if (NULL == pMem)
    {
        return BS_FULL;
    }

    Mem_Zero((UCHAR*)pMem + uiOldSize * sizeof(HANDLE), (uiNewSize - uiOldSize) * sizeof(HANDLE));

    if (NULL != pstCtrl->phArray)
    {
        MEM_Free(pstCtrl->phArray);
    }

    pstCtrl->phArray = pMem;
    pstCtrl->uiSize = uiNewSize;

    return BS_OK;
}

static UINT darray_AddToSpace(_DARRAY_CTRL_S *pstCtrl, IN HANDLE hData)
{
    UINT i;

    for (i=0; i<pstCtrl->uiSize; i++)
    {
        if (pstCtrl->phArray[i] == NULL)
        {
            pstCtrl->phArray[i] = hData;
            return i;
        }
    }

    return DARRAY_INVALID_INDEX;
}

/* 返回将数据添加到的Index. */
static UINT darray_ExpandAndAdd(IN _DARRAY_CTRL_S *pstCtrl, IN HANDLE hData)
{
    UINT uiIndex;

    BS_DBGASSERT(NULL != pstCtrl);

    uiIndex = pstCtrl->uiSize - 1;

    if (BS_OK != darray_Expand(pstCtrl, pstCtrl->uiSize + _DARRAY_DFT_STEP_SIZE))
    {
        return DARRAY_INVALID_INDEX;
    }

    pstCtrl->phArray[uiIndex] = hData;

    return uiIndex;
}

DARRAY_HANDLE DARRAY_Create(IN UINT uiStaticNum/* 静态分配的个数 */)
{
    _DARRAY_CTRL_S *pstCtrl;

    pstCtrl = MEM_ZMalloc(sizeof(_DARRAY_CTRL_S));
    if (NULL == pstCtrl)
    {
        return NULL;
    }

    if (uiStaticNum != 0)
    {
        pstCtrl->phArray = MEM_ZMalloc(sizeof(HANDLE) * uiStaticNum);
        if (NULL == pstCtrl->phArray)
        {
            MEM_Free(pstCtrl);
            return NULL;
        }
        pstCtrl->uiSize = uiStaticNum;
    }

    return pstCtrl;
}

VOID DARRAY_Destory(IN DARRAY_HANDLE hDArray)
{
    _DARRAY_CTRL_S *pstCtrl = hDArray;

    if (NULL != pstCtrl)
    {
        if (NULL != pstCtrl->phArray)
        {
            MEM_Free(pstCtrl->phArray);
        }

        MEM_Free(pstCtrl);
    }
}

/* 返回将数据添加到的Index */
UINT DARRAY_Add(IN DARRAY_HANDLE hDArray, IN HANDLE hData)
{
    UINT uiIndex;
    _DARRAY_CTRL_S *pstCtrl = hDArray;

    BS_DBGASSERT(NULL != pstCtrl);
    BS_DBGASSERT(NULL != hData);

    uiIndex = darray_AddToSpace(pstCtrl, hData);
    if (uiIndex == DARRAY_INVALID_INDEX)
    {
        uiIndex = darray_ExpandAndAdd(hDArray, hData);
    }

    if (uiIndex != DARRAY_INVALID_INDEX) {
        pstCtrl->uiCount ++;
    }

    return uiIndex;
}

BS_STATUS DARRAY_Set(IN DARRAY_HANDLE hDArray, IN UINT uiIndex, IN HANDLE hData)
{
    _DARRAY_CTRL_S *pstCtrl = hDArray;

    BS_DBGASSERT(NULL != hData);

    if (uiIndex >= pstCtrl->uiSize)
    {
        if (BS_OK != darray_Expand(pstCtrl, uiIndex + 1))
        {
            return BS_NO_MEMORY;
        }
    }

    if (NULL == pstCtrl->phArray[uiIndex])
    {
        pstCtrl->uiCount ++;
    }

    pstCtrl->phArray[uiIndex] = hData;

    return BS_OK;
}

HANDLE DARRAY_Get(IN DARRAY_HANDLE hDArray, IN UINT uiIndex)
{
    _DARRAY_CTRL_S *pstCtrl = hDArray;

    if (uiIndex >= pstCtrl->uiSize)
    {
        return NULL;
    }

    return pstCtrl->phArray[uiIndex];
}

HANDLE DARRAY_Del(IN DARRAY_HANDLE hDArray, IN UINT uiIndex)
{
    _DARRAY_CTRL_S *pstCtrl = hDArray;
    HANDLE hData;

    if (uiIndex >= pstCtrl->uiSize)
    {
        return NULL;
    }

    hData = pstCtrl->phArray[uiIndex];
    if (NULL != hData)
    {
        pstCtrl->uiCount --;
        pstCtrl->phArray[uiIndex] = NULL;
    }

    return hData;
}

/* 得到有多少个数据 */
UINT DARRAY_GetCount(IN DARRAY_HANDLE hDArray)
{
    _DARRAY_CTRL_S *pstCtrl = hDArray;

    return pstCtrl->uiCount;
}

/* 得到现在动态数组的大小 */
UINT DARRAY_GetSize(IN DARRAY_HANDLE hDArray)
{
    _DARRAY_CTRL_S *pstCtrl = hDArray;

    return pstCtrl->uiSize;
}


