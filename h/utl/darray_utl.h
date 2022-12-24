/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-11-16
* Description: 
* History:     
******************************************************************************/

#ifndef __DARRAY_UTL_H_
#define __DARRAY_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define DARRAY_INVALID_INDEX 0xffffffff


typedef HANDLE DARRAY_HANDLE;

DARRAY_HANDLE DARRAY_Create(UINT init_num/* 初始分配的个数 */, USHORT step_num);
VOID DARRAY_Destory(IN DARRAY_HANDLE hDArray);
/* 返回将数据添加到的Index, 不支持hData为0 */
UINT DARRAY_Add(IN DARRAY_HANDLE hDArray, IN HANDLE hData);
BS_STATUS DARRAY_Set(IN DARRAY_HANDLE hDArray, IN UINT uiIndex, IN HANDLE hData);
HANDLE DARRAY_Get(IN DARRAY_HANDLE hDArray, IN UINT uiIndex);
HANDLE DARRAY_Clear(IN DARRAY_HANDLE hDArray, IN UINT uiIndex);
/* 得到现在动态数组的大小 */
UINT DARRAY_GetSize(IN DARRAY_HANDLE hDArray);
UINT DARRAY_FindIntData(IN DARRAY_HANDLE hDArray, IN HANDLE hData);


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__DARRAY_UTL_H_*/


