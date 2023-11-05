/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-11-16
* Description: 
* History:     
******************************************************************************/

#ifndef __NAP_INNER_H_
#define __NAP_INNER_H_

#include "utl/large_bitmap.h"

#ifdef __cplusplus
    extern "C" {
#endif 

typedef VOID (*PF_NAP_DESTORY)(IN NAP_HANDLE hNAPHandle);
typedef VOID* (*PF_NAP_Alloc)(IN HANDLE hNapHandle, IN UINT uiIndex);
typedef VOID (*PF_NAP_FREE)(IN NAP_HANDLE hNapHandle, IN VOID *pstNapNode, IN UINT uiIndex);
typedef VOID* (*PF_NAP_GetNodeByIndex)(IN NAP_HANDLE hNAPHandle, IN UINT ulID);

typedef struct
{
    BOOL_T bEnable;
    UINT   ulSeqMask;
    UCHAR  ucSeqStartIndex;
    USHORT *seq_array;  
    UINT   uiSeqArrayCount;  
}_NAP_SEQ_OPT_S;

typedef struct
{
    UINT64 ulPrefix;
    UINT64 ulPrefixMask;
}_NAP_PREFIX_OPT_S;

typedef struct
{
    PF_NAP_DESTORY pfDestory;
    PF_NAP_Alloc pfAlloc;
    PF_NAP_FREE pfFree;
    PF_NAP_GetNodeByIndex pfGetNodeByIndex;
}_NAP_FUNC_TBL_S;


typedef struct
{
    _NAP_FUNC_TBL_S *pstFuncTbl;
    UINT uiMaxNum;
    UINT uiNodeSize;
    UINT uiFlag;
    UINT uiCount;
    UINT ulIndexMask; 
    LBITMAP_HANDLE hLBitmap;
    _NAP_SEQ_OPT_S stSeqOpt;
    void *memcap;
}_NAP_HEAD_COMMON_S;

_NAP_HEAD_COMMON_S * _NAP_ArrayCreate(NAP_PARAM_S *p);
_NAP_HEAD_COMMON_S * _NAP_PtrArrayCreate(NAP_PARAM_S *p);
_NAP_HEAD_COMMON_S * _NAP_HashCreate(NAP_PARAM_S *p);
_NAP_HEAD_COMMON_S * _NAP_AvlCreate(NAP_PARAM_S *p);

#ifdef __cplusplus
    }
#endif 

#endif 


