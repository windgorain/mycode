/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2008-3-19
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/log_utl.h"
#include "utl/hookapi.h"

#include "hookapi_os.h"

typedef struct tagHookApiList
{
    DLL_NODE(tagHookApiList) stDllNode;
    CHAR *pszCalleeName;
    UINT ulOldEntry;
    UINT ulNewEntry;
}_HOOK_API_LIST_S;

typedef DLL_HEAD(_HOOK_API_LIST_S) _HOOK_API_LIST_HEAD_S;

static _HOOK_API_LIST_HEAD_S g_stHookApiListHead = DLL_HEAD_INIT_VALUE(&g_stHookApiListHead);
static HANDLE                g_hHookApiLogId = 0;


HANDLE _HOOKAPI_GetLogId()
{
    if (g_hHookApiLogId == 0)
    {
        g_hHookApiLogId = LogFile_Open("hookapi_log.txt");
    }

    return g_hHookApiLogId;
}

VOID HOOKAPI_ReplaceIATEntryInOne
(
    IN CHAR *pszCallerModuleName,
	IN HANDLE hCallerHandle,
	IN CHAR *pszCalleeModName,
	IN HANDLE pfCurrentEntry,
	IN HANDLE pfNewEntry
)
{
    OS_HOOKAPI_ReplaceIATEntryInOne(pszCallerModuleName, hCallerHandle, pszCalleeModName, pfCurrentEntry, pfNewEntry);
}

VOID HOOKAPI_ReplaceIATEntryInAll
(
    IN CHAR *pszCalleeModName,
    IN HANDLE pfCurrentEntry,
    IN HANDLE pfNewEntry,
    IN BOOL_T bIfExcludeAPIHookMod
)
{
    OS_HOOKAPI_ReplaceIATEntryInAll(pszCalleeModName, pfCurrentEntry, pfNewEntry, bIfExcludeAPIHookMod);
}

static VOID HOOKAPI_ReplaceIATEntryInAllButOne
(
    IN CHAR *pszCalleeModName,
    IN HANDLE pfCurrentEntry,
    IN HANDLE pfNewEntry,
    IN BOOL_T bIfExcludeAPIHookMod,
    IN UINT ulButOneHandle
)
{
    OS_HOOKAPI_ReplaceIATEntryInAllButOne(pszCalleeModName, pfCurrentEntry, pfNewEntry, bIfExcludeAPIHookMod, ulButOneHandle);
}

UINT HOOKAPI_GetModuleHandle(IN CHAR *pszModeName)
{
    return OS_HookApi_GetModuleHandle(pszModeName);
}

VOID HOOKAPI_ReplaceIATByTable(IN HOOKAPI_ENTRY_TBL_S *pstTbl, IN UINT ulNum)
{
    UINT i;
    PLUG_ID ulHandle;
    HANDLE pfOldEntry;

    for (i=0; i<ulNum; i++)
    {
        if (pstTbl[i].pfOldFuncAddr != 0)
        {
            continue;
        }
        
        ulHandle = (PLUG_ID)HOOKAPI_GetModuleHandle(pstTbl[i].pszDllName);
        if (0 == ulHandle)
        {
            continue;
        }
        
        pfOldEntry = PLUG_GET_FUNC_BY_NAME(ulHandle, pstTbl[i].pszOldFuncName);
        if (pfOldEntry == 0)
        {
            continue;
        }

        pstTbl[i].pfOldFuncAddr = pfOldEntry;
        LogFile_OutString(_HOOKAPI_GetLogId(), "\r\n%s:%s:0x%p",
            pstTbl[i].pszDllName, pstTbl[i].pszOldFuncName, pfOldEntry);
    }

    OS_HOOKAPI_ReplaceSysFunc(pstTbl, ulNum);

    for (i = 0; i<ulNum; i++)
    {
        if (pstTbl[i].pfOldFuncAddr == 0)
        {
            continue;
        }

        HOOKAPI_ReplaceIATEntryInAll(pstTbl[i].pszDllName, pstTbl[i].pfOldFuncAddr, pstTbl[i].pfNewFuncEntry, FALSE);
    }
}


