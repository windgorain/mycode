/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-7-23
* Description: 
* History:     
******************************************************************************/

#ifndef __SVPN_CTXDATA_H_
#define __SVPN_CTXDATA_H_

#include "utl/string_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef enum
{
    SVPN_CTXDATA_LOCAL_USER = 0,
    SVPN_CTXDATA_ADMIN,
    SVPN_CTXDATA_ACL,
    SVPN_CTXDATA_ROLE,
    SVPN_CTXDATA_WEB_RES,
    SVPN_CTXDATA_TCP_RES,
    SVPN_CTXDATA_IP_RES,
    SVPN_CTXDATA_IPPOOL,
    SVPN_CTXDATA_InnerDNS,

    SVPN_CONTEXT_DATA_MAX
}SVPN_CTXDATA_E;

BS_STATUS SVPN_CtxData_Init();
BS_STATUS SVPN_CtxData_AddObject(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN SVPN_CTXDATA_E enDataIndex, IN CHAR *pcTag);
BS_STATUS SVPN_CtxData_GetNextObject
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR *pcCurrentTag,
    OUT CHAR *pcNextTag,
    IN UINT uiNextTagMaxSize
);
BS_STATUS SVPN_CtxData_SetProp
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR *pcTag,
    IN CHAR *pcProp,
    IN CHAR *pcPropValue
);
BS_STATUS SVPN_CtxData_GetProp
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR *pcTag,
    IN CHAR *pcProp,
    OUT CHAR *pcPropValue,
    IN UINT uiPropValueMaxSize
);
HSTRING SVPN_CtxData_GetPropAsHString
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR *pcTag,
    IN CHAR *pcProp
);
BOOL_T SVPN_CtxData_IsObjectExist(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN SVPN_CTXDATA_E enDataIndex, IN CHAR *pcTag);
VOID SVPN_CtxData_DelObject(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN SVPN_CTXDATA_E enDataIndex, IN CHAR *pcTag);
UINT SVPN_CtxData_GetObjectCount(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN SVPN_CTXDATA_E enDataIndex);
/* 删除指定表里面所有行的Prop里面的一项元素. 这个prop必须是以逗号分隔元素的 */
VOID SVPN_CtxData_AllDelPropElement
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR *pcPropName,
    IN CHAR *pcPropElement,
    IN CHAR cSplit
);
VOID SVPN_CtxData_DelPropElement
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR *pcTag,
    IN CHAR *pcPropName,
    IN CHAR *pcPropElement,
    IN CHAR cSplit
);
BS_STATUS SVPN_CtxData_AddPropElement
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR *pcTag,
    IN CHAR *pcPropName,
    IN CHAR *pcPropElement,
    IN CHAR cSplit
);




BS_STATUS SVPN_CD_EnterView
(
    IN VOID *pEnv,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR *pcName
);
BS_STATUS SVPN_CD_SetProp
(
    IN VOID *pEnv,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR *pcProp,
    IN CHAR *pcPropValue
);
BS_STATUS SVPN_CD_AddPropElement
(
    IN VOID *pEnv,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR *pcProp,
    IN CHAR *pcElement,
    IN CHAR cSplit
);
VOID SVPN_CD_SaveProp
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR *pcName,
    IN CHAR *pcProp,
    IN CHAR *pcCmdPrefix,
    IN HANDLE hFile
);
VOID SVPN_CD_SaveBoolProp
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR *pcName,
    IN CHAR *pcProp,
    IN CHAR *pcCmdPrefix,
    IN HANDLE hFile
);
VOID SVPN_CD_SaveElements
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR *pcName,
    IN CHAR *pcProp,
    IN CHAR *pcCmdPrefix,
    IN CHAR cSplit,
    IN HANDLE hFile
);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__SVPN_CTXDATA_H_*/


