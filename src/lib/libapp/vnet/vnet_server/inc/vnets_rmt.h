/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-10-9
* Description: 
* History:     
******************************************************************************/

#ifndef __VNETS_RMT_H_
#define __VNETS_RMT_H_

#include "utl/string_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */


int VNETS_RMT_DomainReboot(U64 p1);
UINT VNETS_RMT_DomainGetNextNode(U64 p1, U64 p2);
HSTRING VNETS_RMT_GetNodeInfo(U64 p1);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETS_RMT_H_*/


