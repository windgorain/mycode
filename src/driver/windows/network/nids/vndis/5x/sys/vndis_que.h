/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-3-30
* Description: 
* History:     
******************************************************************************/

#ifndef __VNDIS_QUE_H_
#define __VNDIS_QUE_H_

#ifdef __cplusplus
    extern "C" {
#endif 

typedef HANDLE VNDIS_QUE_HANDLE;

VNDIS_QUE_HANDLE VNDIS_QUE_Create(IN UINT uiCapacity);
VOID VNDIS_QUE_Destory(IN VNDIS_QUE_HANDLE hQue);
UINT VNDIS_QUE_Count(IN VNDIS_QUE_HANDLE hQue);
BOOLEAN VNDIS_QUE_IsFull(IN VNDIS_QUE_HANDLE hQue);
VOID * VNDIS_QUE_Push(IN VNDIS_QUE_HANDLE hQue, IN VOID *pItem);
VOID * VNDIS_QUE_Pop(IN VNDIS_QUE_HANDLE hQue);
VOID * VNDIS_QUE_ExtractPop(IN VNDIS_QUE_HANDLE hQue, IN VOID *pItem);


#ifdef __cplusplus
    }
#endif 

#endif 



