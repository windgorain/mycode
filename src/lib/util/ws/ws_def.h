/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-8-10
* Description: 
* History:     
******************************************************************************/

#ifndef __WS_DEF_H_
#define __WS_DEF_H_

#include "utl/file_utl.h"
#include "utl/drp_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#if 1  /* WS */

#define _WS_DEBUG_SWICH_TEST(_pstWs, _X) ((_pstWs)->uiDbgFlag & (_X))

#define _WS_DEBUG_PKT(_pstWs, _X) BS_DBG_OUTPUT((_pstWs)->uiDbgFlag, WS_DBG_PACKET, _X)
#define _WS_DEBUG_EVENT(_pstWs, _X) BS_DBG_OUTPUT((_pstWs)->uiDbgFlag, WS_DBG_EVENT, _X)
#define _WS_DEBUG_PROCESS(_pstWs, _X) BS_DBG_OUTPUT((_pstWs)->uiDbgFlag, WS_DBG_PROCESS, _X)
#define _WS_DEBUG_ERR(_pstWs, _X) BS_DBG_OUTPUT((_pstWs)->uiDbgFlag, WS_DBG_ERR, _X)

typedef struct
{
    WS_FUNC_TBL_S stFuncTbl;
    UINT uiDbgFlag;
    DLL_HEAD_S stVHostList;
}_WS_S;

typedef enum
{
    WS_PLUG_SET_CONTEXT = 0,
    WS_PLUG_CONTEXT,
    WS_PLUG_INDEX,
    WS_PLUG_DELIVER,
    WS_PLUG_DWEB,
    WS_PLUG_STATIC,

    WS_PLUG_MAX
}WS_PLUG_E;

#endif

#if 1 /* Context */

typedef struct
{
    DLL_NODE_S stLinkNode;

    CHAR szDomain[WS_DOMAIN_MAX_LEN + 1];
    UINT uiDomainLen;
    CHAR szRootPath[FILE_MAX_PATH_LEN + 1];
    CHAR szSecondRootPath[FILE_MAX_PATH_LEN + 1];  /* 当在root path中找不到对应文件时，到SecondRootPath中再找一次. */
    CHAR szIndex[WS_CONTEXT_MAX_INDEX_LEN + 1];
    VOID *pVHost;
    WS_DELIVER_TBL_HANDLE hDeliverTbl;
    VOID *pUserData;
}_WS_CONTEXT_S;

typedef struct
{
    _WS_CONTEXT_S stDftContext;
    DLL_HEAD_S stContextList;
}_WS_CONTEXT_CONTAINER_S;

#endif

#if 1 /* VHost */

typedef struct
{
    DLL_NODE_S stLinkNode;

    CHAR szVHost[WS_VHOST_MAX_LEN + 1];
    UINT uiVHostLen;
    _WS_CONTEXT_CONTAINER_S stContexts;
    _WS_S *pstWs;
}_WS_VHOST_S;

#endif



#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__WS_DEF_H_*/


