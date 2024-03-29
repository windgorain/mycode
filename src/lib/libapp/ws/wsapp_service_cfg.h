/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2015-12-3
* Description: 
* History:     
******************************************************************************/

#ifndef __WSAPP_SERVICE_CFG_H_
#define __WSAPP_SERVICE_CFG_H_

#include "utl/file_utl.h"
#include "utl/ws_utl.h"
#include "comp/comp_wsapp.h"
#include "wsapp_def.h"

#ifdef __cplusplus
    extern "C" {
#endif 

#define WSAPP_SERVICE_MAX_BIND_NUM 10  

typedef struct
{
    CHAR szBindGwName[WSAPP_GW_NAME_LEN + 1];
    CHAR szVHostName[WS_VHOST_MAX_LEN + 1];
    CHAR szDomain[WS_DOMAIN_MAX_LEN + 1];
    WS_CONTEXT_HANDLE hWsContext;
}WSAPP_SERVICE_BIND_INFO_S;


typedef struct
{
    
    WSAPP_SERVICE_BIND_INFO_S astBindGateWay[WSAPP_SERVICE_MAX_BIND_NUM];
    BOOL_T bStart;

    
    CHAR szRootPath[FILE_MAX_PATH_LEN + 1];
    CHAR szIndex[WS_CONTEXT_MAX_INDEX_LEN + 1];

    
    UINT uiFlag;
    UINT64 ulUserData;
    WS_DELIVER_TBL_HANDLE hDeliverTbl;
}WSAPP_SERVICE_S;

#ifdef __cplusplus
    }
#endif 

#endif 


