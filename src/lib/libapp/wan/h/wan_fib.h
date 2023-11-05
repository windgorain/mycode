/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-3-26
* Description: 
* History:     
******************************************************************************/

#ifndef __WAN_FIB_H_
#define __WAN_FIB_H_

#include "utl/fib_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif 

BS_STATUS WanFib_Init();
BS_STATUS WanFib_PrefixMatch(IN UINT uiVFID, IN UINT uiDstIp , OUT FIB_NODE_S *pstFibNode);
VOID WAN_FIB_Save(IN HANDLE hFile);
BS_STATUS WanFib_KfInit();

#ifdef __cplusplus
    }
#endif 

#endif 


