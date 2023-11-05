/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-6-18
* Description: 
* History:     
******************************************************************************/

#ifndef __SVPN_ACL_H_
#define __SVPN_ACL_H_

#include "utl/uri_acl.h"

#ifdef __cplusplus
    extern "C" {
#endif 


BS_STATUS SVPN_ACL_Init();


BS_STATUS SVPN_ACL_Match
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN CHAR *pcAclListName,
    IN URI_ACL_MATCH_INFO_S *pstMatchInfo,
    OUT BOOL_T *pbPermit
);

#ifdef __cplusplus
    }
#endif 

#endif 


