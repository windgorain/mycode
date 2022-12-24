/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2008-3-20
* Description: 
* History:     
******************************************************************************/

#ifndef __HOOKAPI_H_
#define __HOOKAPI_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef struct
{
    CHAR * pszDllName;
    CHAR * pszOldFuncName;
    HANDLE pfOldFuncAddr;
    HANDLE pfNewFuncEntry;
}HOOKAPI_ENTRY_TBL_S;

extern VOID HOOKAPI_ReplaceIATEntryInOne
(
    IN CHAR *pszCallerModuleName,
	IN HANDLE hCallerHandle,
	IN CHAR *pszCalleeModName,
	IN HANDLE pfCurrentEntry,
	IN HANDLE pfNewEntry
);

extern VOID HOOKAPI_ReplaceIATEntryInAll
(
    IN CHAR *pszCalleeModName,
    IN HANDLE pfCurrentEntry,
    IN HANDLE pfNewEntry,
    IN BOOL_T bIfExcludeAPIHookMod
);

extern VOID HOOKAPI_ReplaceIATEntryInAllButOne
(
    IN CHAR *pszCalleeModName,
    IN HANDLE pfCurrentEntry,
    IN HANDLE pfNewEntry,
    IN BOOL_T bIfExcludeAPIHookMod,
    IN UINT ulButOneHandle
);

extern UINT HOOKAPI_GetModuleHandle(IN CHAR *pszModeName);

extern VOID HOOKAPI_ReplaceIATByTable(IN HOOKAPI_ENTRY_TBL_S *pstTbl, IN UINT ulNum);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__HOOKAPI_H_*/


