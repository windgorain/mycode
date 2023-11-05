/******************************************************************************
* Copyright (C), 2000-2006,  Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2015-6-1
* Description: 
* History:     
******************************************************************************/

#ifndef __SVPN_MF_H_
#define __SVPN_MF_H_

#include "utl/kf_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif 

typedef VOID (*PF_svpn_mf_map_func)(IN MIME_HANDLE hMime, IN SVPN_DWEB_S *pstDweb);

typedef struct
{
    UINT uiRightFlag;
    CHAR *pcKey;
    PF_svpn_mf_map_func pfFunc;
}SVPN_MF_MAP_S;

BS_STATUS SVPN_MF_Init();
BS_STATUS SVPN_MF_Reg(IN SVPN_MF_MAP_S *pstMap, IN UINT uiMapCount);
BS_STATUS SVPN_MF_Run(IN SVPN_DWEB_S *pstDweb);

static inline VOID svpn_mf_SetSuccess(IN SVPN_DWEB_S *pstDweb)
{
    cJSON_AddStringToObject(pstDweb->pstJson, "result", "Success");
}

static inline VOID svpn_mf_SetFailed(IN SVPN_DWEB_S *pstDweb, IN CHAR *pcReason)
{
    cJSON_AddStringToObject(pstDweb->pstJson, "result", "Failed");
    cJSON_AddStringToObject(pstDweb->pstJson, "reason", pcReason);
}

typedef VOID (*PF_svpn_mf_CommonDeleteNotify)(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcName);


VOID SVPN_MF_CommonIsExist
(
    IN MIME_HANDLE hMime,
    IN SVPN_DWEB_S *pstDweb,
    IN SVPN_CTXDATA_E enDataIndex
);
BS_STATUS SVPN_MF_CommonAdd
(
    IN MIME_HANDLE hMime,
    IN SVPN_DWEB_S *pstDweb,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR **apcPropertys,
    IN UINT uiPropertyCount
);
VOID SVPN_MF_CommonModify
(
    IN MIME_HANDLE hMime,
    IN SVPN_DWEB_S *pstDweb,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR **apcPropertys,
    IN UINT uiPropertyCount
);
VOID SVPN_MF_CommonGet
(
    IN MIME_HANDLE hMime,
    IN SVPN_DWEB_S *pstDweb,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR **apcPropertys,
    IN UINT uiPropertyCount
);
VOID SVPN_MF_CommonDelete
(
    IN MIME_HANDLE hMime,
    IN SVPN_DWEB_S *pstDweb,
    IN SVPN_CTXDATA_E enDataIndex,
    IN PF_svpn_mf_CommonDeleteNotify pfDeleteNotify
);
VOID SVPN_MF_CommonList
(
    IN MIME_HANDLE hMime,
    IN SVPN_DWEB_S *pstDweb,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR **apcPropertys,
    IN UINT uiPropertyCount
);


#ifdef __cplusplus
    }
#endif 

#endif 


