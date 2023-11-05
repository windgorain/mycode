/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2008-3-19
* Description: 
* History:     
******************************************************************************/

#ifndef __HOOKAPI_OS_H_
#define __HOOKAPI_OS_H_

#ifdef __cplusplus
    extern "C" {
#endif 

extern VOID OS_HOOKAPI_ReplaceIATEntryInOne
(
    IN CHAR *pszCallerModuleName,
	IN HANDLE hCallerHandle,
	IN CHAR *pszCalleeModName,
	IN HANDLE pfCurrentEntry,
	IN HANDLE pfNewEntry
);

extern VOID OS_HOOKAPI_ReplaceIATEntryInAll
(
    IN CHAR *pszCalleeModName,
    IN HANDLE pfnCurrent,
    IN HANDLE pfnNew,
    IN BOOL_T bIfExcludeAPIHookMod
);

VOID OS_HOOKAPI_ReplaceIATEntryInAllButOne
(
    IN CHAR *pszCalleeModName,
    IN HANDLE pfnCurrent,
    IN HANDLE pfnNew,
    IN BOOL_T bIfExcludeAPIHookMod,
    IN UINT ulButOneHandle
);

VOID OS_HOOKAPI_ReplaceSysFunc();

UINT OS_HookApi_GetModuleHandle(IN CHAR *pszModeName);

HANDLE _HOOKAPI_GetLogId();




#ifdef __cplusplus
    }
#endif 

#endif 


