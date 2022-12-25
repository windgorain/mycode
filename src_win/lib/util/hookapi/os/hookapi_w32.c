/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2008-3-19
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/log_utl.h"
#include "utl/hookapi.h"

#ifdef IN_WINDOWS

#include <tlhelp32.h> 
#include <Dbghelp.h>
#include "../hookapi_os.h"
#include "../toolhelp_os.h"


const UCHAR cPushOpCode = 0x68;   // The PUSH opcode on x86 platforms

HMODULE WINAPI _OS_LoadLibraryA_Hook(IN CHAR * pszFileName);
HMODULE WINAPI _OS_LoadLibraryW_Hook(IN CHAR * pszFileName);
HMODULE WINAPI _OS_LoadLibraryExA_Hook(IN CHAR * pszFileName, IN UINT hFile, IN UINT ulFlag);
HMODULE WINAPI _OS_LoadLibraryExW_Hook(IN CHAR * pszFileName, IN UINT hFile, IN UINT ulFlag);
FARPROC WINAPI _OS_GetProcAddress_HOOK(IN UINT ulPlugId, IN CHAR *pszFuncName);

static HOOKAPI_ENTRY_TBL_S g_stHookApiSysFunc[] = 
{
    {"kernel32.dll", "LoadLibraryA", 0, (HANDLE)_OS_LoadLibraryA_Hook},
    {"kernel32.dll", "LoadLibraryW", 0, (HANDLE)_OS_LoadLibraryW_Hook},
    {"kernel32.dll", "LoadLibraryExA", 0, (HANDLE)_OS_LoadLibraryExA_Hook},
    {"kernel32.dll", "LoadLibraryExW", 0, (HANDLE)_OS_LoadLibraryExW_Hook},
    {"kernel32.dll", "GetProcAddress", 0, (HANDLE)_OS_GetProcAddress_HOOK},
};

static HOOKAPI_ENTRY_TBL_S *g_pstOsHookApiUsrTbl;
static UINT                g_ulOsHookAPiUsrTblSize;



VOID OS_HOOKAPI_ReplaceIATEntryInOne
(
    IN CHAR *pszCallerModuleName,
	IN HANDLE hCallerHandle,
	IN CHAR *pszCalleeModName,
	IN HANDLE pfCurrentEntry,
	IN HANDLE pfNewEntry
)
{
    UINT ulSize;
    PIMAGE_IMPORT_DESCRIPTOR pstImportDesc;
    PIMAGE_THUNK_DATA pThunk;
    PROC* ppfn;
    BOOL fFound;
    PBYTE pbInFunc;
    SYSTEM_INFO si;
    static UINT ulMaxAppAddr = 0;
    UINT ulRet;
    UINT dwOldProtect;

    if (stricmp(pszCallerModuleName, "ws2_32.dll") == 0)
    {
        return;
    }

    if (ulMaxAppAddr == 0)
    {
        GetSystemInfo(&si);
        ulMaxAppAddr = (UINT)si.lpMaximumApplicationAddress;
    }

    /* 得到输入节 */
    pstImportDesc = (PIMAGE_IMPORT_DESCRIPTOR) 
    		ImageDirectoryEntryToData((PVOID)hCallerHandle, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &ulSize);

    if (pstImportDesc == NULL)  /* 没有输入节 */
    {
    	return;
    }
    
    /* 查找对应的引入DLL */
    for (; pstImportDesc->Name != 0; pstImportDesc++)
    {
        CHAR * pszModName = (CHAR *) ((PBYTE) hCallerHandle + pstImportDesc->Name);

        if (stricmp(pszModName, pszCalleeModName) == 0)
        {
            break;
        }
    }

    if (pstImportDesc->Name == 0)
    {
        return;  /* 没有对应的引入DLL */
    }

    /* 得到引用函数列表 */
    pThunk = (PIMAGE_THUNK_DATA) ((PBYTE) hCallerHandle + pstImportDesc->FirstThunk);

    /* 查找对应函数 */
    for (; pThunk->u1.Function; pThunk++)
    {
        /* 得到函数对应地址 */
        ppfn = (PROC*) &pThunk->u1.Function;

        // Is this the function we're looking for?
        fFound = ((HANDLE)*ppfn == pfCurrentEntry);

        if (!fFound && ((UINT)*ppfn > ulMaxAppAddr))
        {
            // If this is not the function and the address is in a shared DLL, 
            // then maybe we're running under a debugger on Windows 98. In this 
            // case, this address points to an instruction that may have the 
            // correct address.

            pbInFunc = (PBYTE) *ppfn;
            if (pbInFunc[0] == cPushOpCode)
            {
                // We see the PUSH instruction, the real function address follows
                ppfn = (PROC*) &pbInFunc[1];

                // Is this the function we're looking for?
                fFound = ((HANDLE)*ppfn == pfCurrentEntry);
            }
        }

        if (fFound)
        {
            // The addresses match, change the import section address
            VirtualProtect(ppfn,sizeof(pfNewEntry), PAGE_READWRITE, &dwOldProtect); 
            ulRet = WriteProcessMemory(GetCurrentProcess(), ppfn, &pfNewEntry, sizeof(pfNewEntry), NULL);
            VirtualProtect(ppfn,sizeof(pfNewEntry),dwOldProtect,0);

            if (0 == ulRet)
            {
                LogFile_OutString(_HOOKAPI_GetLogId(), "\r\nFailed to replace %s:0x%p to 0x%p in %s",
                        pszCalleeModName, pfCurrentEntry, pfNewEntry, pszCallerModuleName);
            }
            else
            {
                LogFile_OutString(_HOOKAPI_GetLogId(), "\r\nReplace %s:0x%p to 0x%p in %s",
                    pszCalleeModName, pfCurrentEntry, pfNewEntry, pszCallerModuleName);
            }
            
            return;
        }
    }
}

// Returns the HMODULE that contains the specified memory address
static HMODULE _OS_HOOKAPI_ModuleFromAddress(PVOID pv)
{
   MEMORY_BASIC_INFORMATION mbi;
   return((VirtualQuery(pv, &mbi, sizeof(mbi)) != 0) ? (HMODULE) mbi.AllocationBase : NULL);
}

VOID OS_HOOKAPI_ReplaceIATEntryInAll
(
    IN CHAR *pszCalleeModName,
    IN HANDLE pfnCurrent,
    IN HANDLE pfnNew,
    IN BOOL_T bIfExcludeAPIHookMod
)
{
    HMODULE hmodThisMod;
    MODULEENTRY32 me;
    UINT ulToolHelpId;
    BS_STATUS eRet;

    hmodThisMod = bIfExcludeAPIHookMod ? _OS_HOOKAPI_ModuleFromAddress(OS_HOOKAPI_ReplaceIATEntryInAll) : NULL;

    if (ToolHelp_CreateModule(GetCurrentProcessId(), &ulToolHelpId) != BS_OK)
    {
        return;
    }

    for (eRet = ToolHelp_GetFirstModule(ulToolHelpId, &me);
        eRet == BS_OK;
        eRet = ToolHelp_GetNextModule(ulToolHelpId, &me))
    {
        if (me.hModule != hmodThisMod)
        {
            OS_HOOKAPI_ReplaceIATEntryInOne((CHAR*)me.szModule, me.hModule, pszCalleeModName, pfnCurrent, pfnNew);
        }
    }

    ToolHelp_Delete(ulToolHelpId);
}


VOID OS_HOOKAPI_ReplaceIATEntryInAllButOne
(
    IN CHAR *pszCalleeModName,
    IN HANDLE pfnCurrent,
    IN HANDLE pfnNew,
    IN BOOL_T bIfExcludeAPIHookMod,
    IN UINT ulButOneHandle
)
{
    HMODULE hmodThisMod;
    MODULEENTRY32 me;
    UINT ulToolHelpId;
    BS_STATUS eRet;
    
    hmodThisMod = bIfExcludeAPIHookMod ? _OS_HOOKAPI_ModuleFromAddress(OS_HOOKAPI_ReplaceIATEntryInAll) : NULL;

    if (ToolHelp_CreateModule(GetCurrentProcessId(), &ulToolHelpId) != BS_OK)
    {
        return;
    }
    
    for (eRet = ToolHelp_GetFirstModule(ulToolHelpId, &me);
       eRet == BS_OK;
       eRet = ToolHelp_GetNextModule(ulToolHelpId, &me))
    {
        if ((UINT)me.hModule == ulButOneHandle)
        {
            continue;
        }

        if (me.hModule != hmodThisMod)
        {
            OS_HOOKAPI_ReplaceIATEntryInOne((CHAR*)me.szModule, me.hModule, pszCalleeModName, pfnCurrent, pfnNew);
        }
    }

    ToolHelp_Delete(ulToolHelpId);
}

UINT OS_HookApi_GetModuleHandle(IN CHAR *pszModeName)
{
    return (UINT)GetModuleHandle((LPTSTR)pszModeName);
}

HMODULE WINAPI _OS_LoadLibraryA_Hook(IN CHAR * pszFileName)
{
    PLUG_HDL ulPlugId;

    ulPlugId = (PLUG_HDL) APITBL_LoadLibrary(pszFileName);
    if (ulPlugId == 0)
    {
        return 0;
    }
    
    HOOKAPI_ReplaceIATByTable(g_pstOsHookApiUsrTbl, g_ulOsHookAPiUsrTblSize);
    return ulPlugId;
}

HMODULE WINAPI _OS_LoadLibraryExA_Hook(IN CHAR * pszFileName, IN UINT hFile, IN UINT ulFlag)
{
    PLUG_HDL ulPlugId;

    ulPlugId = (PLUG_HDL) APITBL_LoadLibraryExA(pszFileName, (UINT)hFile, ulFlag);
    if (ulPlugId == 0)
    {
        return 0;
    }
    
    HOOKAPI_ReplaceIATByTable(g_pstOsHookApiUsrTbl, g_ulOsHookAPiUsrTblSize);
    return ulPlugId;
}

HMODULE WINAPI _OS_LoadLibraryExW_Hook(IN CHAR * pszFileName, IN UINT hFile, IN UINT ulFlag)
{
    PLUG_HDL ulPlugId;

    ulPlugId = (PLUG_HDL) APITBL_LoadLibraryExW(pszFileName, hFile, ulFlag);
    if (ulPlugId == 0)
    {
        return 0;
    }
    
    HOOKAPI_ReplaceIATByTable(g_pstOsHookApiUsrTbl, g_ulOsHookAPiUsrTblSize);
    return ulPlugId;
}

HMODULE WINAPI _OS_LoadLibraryW_Hook(IN CHAR * pszFileName)
{
    PLUG_HDL ulPlugId;

    ulPlugId = (PLUG_HDL) APITBL_LoadLibraryW(pszFileName);
    if (ulPlugId == 0)
    {
        return 0;
    }

    HOOKAPI_ReplaceIATByTable(g_pstOsHookApiUsrTbl, g_ulOsHookAPiUsrTblSize);
    return ulPlugId;
}

FARPROC WINAPI _OS_GetProcAddress_HOOK(IN UINT ulPlugId, IN CHAR *pszFuncName)
{
    HANDLE pfFunc;
    UINT i;

    pfFunc = PLUG_GET_FUNC_BY_NAME((PLUG_HDL)ulPlugId, pszFuncName);
    if (pfFunc == 0)
    {
        return 0;
    }

    for (i=0; i<g_ulOsHookAPiUsrTblSize; i++)
    {
        if (pfFunc == g_pstOsHookApiUsrTbl[i].pfOldFuncAddr)
        {
            LogFile_OutString(_HOOKAPI_GetLogId(), "\r\nFake 0x%08x to 0x%p",
				pfFunc, g_pstOsHookApiUsrTbl[i].pfNewFuncEntry);
            pfFunc = g_pstOsHookApiUsrTbl[i].pfNewFuncEntry;
            break;
        }
    }

    return (FARPROC)pfFunc;
}

VOID OS_HOOKAPI_ReplaceSysFunc(IN HOOKAPI_ENTRY_TBL_S *pstTbl, IN UINT ulNum)
{
    UINT i;
    UINT ulHandle;
    HANDLE pfOldEntry;

    g_pstOsHookApiUsrTbl = pstTbl;
    g_ulOsHookAPiUsrTblSize = ulNum;

    for (i=0; i<sizeof(g_stHookApiSysFunc)/sizeof(HOOKAPI_ENTRY_TBL_S); i++)
    {
        if (g_stHookApiSysFunc[i].pfOldFuncAddr != 0)
        {
            continue;
        }
        
        ulHandle = HOOKAPI_GetModuleHandle(g_stHookApiSysFunc[i].pszDllName);
        if (0 == ulHandle)
        {
            continue;
        }
        
        pfOldEntry = PLUG_GET_FUNC_BY_NAME((PLUG_HDL)ulHandle, g_stHookApiSysFunc[i].pszOldFuncName);
        if (pfOldEntry == 0)
        {
            continue;
        }

        g_stHookApiSysFunc[i].pfOldFuncAddr = pfOldEntry;

        LogFile_OutString(_HOOKAPI_GetLogId(), "\r\n%s:%s:0x%p",
            g_stHookApiSysFunc[i].pszDllName, g_stHookApiSysFunc[i].pszOldFuncName, pfOldEntry);
    }

    for (i=0; i<sizeof(g_stHookApiSysFunc)/sizeof(HOOKAPI_ENTRY_TBL_S); i++)
    {
        if (g_stHookApiSysFunc[i].pfOldFuncAddr == 0)
        {
            continue;
        }

        HOOKAPI_ReplaceIATEntryInAll(g_stHookApiSysFunc[i].pszDllName,
            g_stHookApiSysFunc[i].pfOldFuncAddr, g_stHookApiSysFunc[i].pfNewFuncEntry, FALSE);
    }
}


#endif

