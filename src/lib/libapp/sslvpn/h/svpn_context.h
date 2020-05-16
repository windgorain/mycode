/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-8-23
* Description: 
* History:     
******************************************************************************/

#ifndef __SVPN_CONTEXT_H_
#define __SVPN_CONTEXT_H_

#include "utl/ws_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef enum
{
    SVPN_CONTEXT_DATA_INDEX_CTXDATA = 0,
    SVPN_CONTEXT_DATA_INDEX_ULM,
    SVPN_CONTEXT_DATA_INDEX_ACL,
    SVPN_CONTEXT_DATA_INDEX_IPTUN_IPPOOL,

    SVPN_CONTEXT_DATA_INDEX_MAX
}SVPN_CONTEXT_DATA_INDEX_E;

#define SVPN_CONTEXT_EVENT_CREATE  0x1
#define SVPN_CONTEXT_EVENT_DESTROY 0x2


typedef HANDLE SVPN_CONTEXT_HANDLE;

typedef BS_STATUS (*PF_SVPN_CONTEXT_IssuEvent)(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN UINT uiEvent);

BS_STATUS SVPN_Context_Init();
VOID SVPN_ContextKf_Init();
CHAR * SVPN_Context_GetName(IN SVPN_CONTEXT_HANDLE hSvpnContext);
VOID SVPN_Context_RegEvent(IN PF_SVPN_CONTEXT_IssuEvent pfFunc);
VOID SVPN_Context_SetUserData(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN UINT uiIndex, IN VOID *pData);
VOID * SVPN_Context_GetUserData(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN UINT uiIndex);
SVPN_CONTEXT_HANDLE SVPN_Context_GetByID(IN UINT64 uiContextID);
SVPN_CONTEXT_HANDLE SVPN_Context_GetByName(IN CHAR *pcName);
CHAR * SVPN_Context_GetNameByID(IN UINT64 uiContextID);
CHAR * SVPN_Context_GetWsService(IN SVPN_CONTEXT_HANDLE hSvpnContext);
CHAR * SVPN_Context_GetDescription(IN SVPN_CONTEXT_HANDLE hSvpnContext);
UINT64 SVPN_Context_GetNextID(IN UINT64 uiCurCtxId);
SVPN_CONTEXT_HANDLE SVPN_Context_GetContextByWsTrans(IN WS_TRANS_HANDLE hWsTrans);


static inline CHAR * SVPN_Context_GetNameByEnv(IN VOID *pEnv, IN UINT uiHistroyIndex)
{
    return CMD_EXP_GetUpModeValue(pEnv, uiHistroyIndex);
}

static inline SVPN_CONTEXT_HANDLE SVPN_Context_GetByEnv(IN VOID *pEnv, IN UINT uiHistroyIndex)
{
    CHAR *pcContextName;

    pcContextName = SVPN_Context_GetNameByEnv(pEnv, uiHistroyIndex);
    if (NULL == pcContextName)
    {
        return NULL;
    }

    return SVPN_Context_GetByName(pcContextName);
}

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__SVPN_CONTEXT_H_*/


