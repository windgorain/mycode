/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2009-4-3
* Description: 
* History:     
******************************************************************************/

#ifndef __VLDCODE_UTL_H_
#define __VLDCODE_UTL_H_

#include "utl/vldbmp_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef enum
{
    VLDCODE_VALID = 0,
    VLDCODE_INVALID,
    VLDCODE_TIMEOUT,
    VLDCODE_NOT_FOUND,
}VLDCODE_RET_E;

HANDLE VLDCODE_CreateInstance(IN UINT ulMaxVldNum);
VOID VLDCODE_DelInstance(IN HANDLE hVldCodeInstance);
/* 产生一个随机的ClientID. 返回0表示失败 */
UINT VLDCODE_RandClientId(IN HANDLE hVldCodeInstance);
VLDBMP_S * VLDCODE_GenVldBmp(IN HANDLE hVldCodeInstance, INOUT UINT *puiClientId/* 当clientID为0时,表示要自动产生一个 */);
VLDCODE_RET_E VLDCODE_Check(IN HANDLE hVldCodeInstance, IN UINT ulClientId, IN CHAR *pszCode);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VLDCODE_UTL_H_*/


