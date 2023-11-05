/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-11-19
* Description: 
* History:     
******************************************************************************/

#ifndef __IDRES_UTL_H_
#define __IDRES_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif 

typedef HANDLE IDRES_HANDLE;

typedef VOID (*PF_IDRES_FREE)(IN UINT uiId, IN HANDLE hRes);

IDRES_HANDLE IDRES_Create(IN PF_IDRES_FREE pfFreeFunc);
VOID IDRES_Destory(IN IDRES_HANDLE hIdRes);
BS_STATUS IDRES_Set(IN IDRES_HANDLE hIdRes, IN UINT uiId, IN HANDLE hRes);
HANDLE IDRES_Get(IN IDRES_HANDLE hIdRes, IN UINT uiId);

#ifdef __cplusplus
    }
#endif 

#endif 


