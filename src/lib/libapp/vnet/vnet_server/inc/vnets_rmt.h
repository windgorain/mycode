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


BS_STATUS VNETS_RMT_DomainReboot(IN CHAR *pcDomainName);
UINT VNETS_RMT_DomainGetNextNode(IN CHAR *pcDomainName, IN UINT uiCurrentNodeId);
HSTRING VNETS_RMT_GetNodeInfo(IN UINT uiNodeID);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETS_RMT_H_*/


