/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-2-17
* Description: 
* History:     
******************************************************************************/

#ifndef __VNETS_WEB_INNER_H_
#define __VNETS_WEB_INNER_H_

#include "utl/ws_utl.h"
#include "utl/cjson.h"

#ifdef __cplusplus
    extern "C" {
#endif 

#define VNETS_WEB_FLAG_EMPTY_JSON 0x1

typedef enum
{
    VNETS_WEB_USER_TYPE_WEB = 0,
    VNETS_WEB_USER_TYPE_CLIENT
}VNETS_WEB_USER_TYPE_E;

typedef struct
{
    UCHAR ucType;
    UINT uiOnlineUserID;
}VNETS_WEB_USER_S;

typedef struct
{
    WS_TRANS_HANDLE hWsTrans;
    UINT uiFlag;
    VNETS_WEB_USER_S stOnlineUser;
    cJSON *pstJson;
}VNETS_WEB_S;

VOID VNETS_Web_BindService(IN CHAR *pcWsService);
BS_STATUS VNETS_WebKf_Run(IN VNETS_WEB_S *pstDweb);

#ifdef __cplusplus
    }
#endif 

#endif 


