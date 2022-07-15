/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-4-8
* Description: 向远程进程插入DLL
* History:     
******************************************************************************/
/* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_INJECTDLL
    
#include "bs.h"

#ifdef IN_WINDOWS
#include "Tlhelp32.h"

static BS_STATUS _OS_INJDLL_EjectDll(IN UINT ulProcessId, IN CHAR *pszDllPath)
{
    HANDLE hthSnapshot = NULL;
    MODULEENTRY32 me = { sizeof(me) };
    BOOL fFound = FALSE;
    BOOL fMoreMods;
    CHAR *pszRemoteDllPath;
    UINT ulDllPathLen = strlen(pszDllPath) + 1;
    PTHREAD_START_ROUTINE pfnThreadRtn;
    HANDLE ulProcessHandle = NULL, ulThreadHandle = NULL;

    hthSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, ulProcessId);
    if (hthSnapshot == NULL)
    {
        return BS_OK;
    }

    /* Get the HMODULE of the desired library */
    fMoreMods = Module32First(hthSnapshot, &me);
    for (; fMoreMods; fMoreMods = Module32Next(hthSnapshot, &me)) {
       fFound = (stricmp((CHAR*)me.szModule,  pszDllPath) == 0) || 
                (stricmp((CHAR*)me.szExePath, pszDllPath) == 0);
       if (fFound) break;
    }

    CloseHandle(hthSnapshot);

    if (!fFound)
    {
        return BS_OK;
    }

    pfnThreadRtn = (PTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(TEXT("Kernel32")), "FreeLibrary");
    if (pfnThreadRtn == NULL)
    {
        RETURN(BS_NO_SUCH);
    }

    ulProcessHandle = OpenProcess(
           PROCESS_QUERY_INFORMATION |   // Required by Alpha
           PROCESS_CREATE_THREAD     |   // For CreateRemoteThread
           PROCESS_VM_OPERATION      |   // For VirtualAllocEx/VirtualFreeEx
           PROCESS_VM_WRITE,             // For WriteProcessMemory
           FALSE, ulProcessId);
    if (ulProcessHandle == NULL)
    {
        RETURN(BS_CAN_NOT_OPEN);
    }

    /* 第一步: 申请远程进程内存 */
    pszRemoteDllPath = VirtualAllocEx(ulProcessHandle, NULL, ulDllPathLen, MEM_COMMIT, PAGE_READWRITE);
    if (NULL == pszRemoteDllPath)
    {
        CloseHandle(ulProcessHandle);
        RETURN(BS_NO_MEMORY);
    }

    if (0 == WriteProcessMemory(ulProcessHandle, pszRemoteDllPath, pszDllPath, ulDllPathLen, NULL))
    {
        VirtualFreeEx(ulProcessHandle, pszRemoteDllPath, 0, MEM_RELEASE);
        CloseHandle(ulProcessHandle);
        RETURN(BS_ERR);
    }

    ulThreadHandle = CreateRemoteThread(ulProcessHandle, NULL, 0, pfnThreadRtn, pszRemoteDllPath, 0, NULL);
    if (ulThreadHandle == NULL)
    {
        VirtualFreeEx(ulProcessHandle, pszRemoteDllPath, 0, MEM_RELEASE);
        CloseHandle(ulProcessHandle);
        RETURN(BS_ERR);
    }

    /* Wait for the remote thread to terminate */
    WaitForSingleObject(ulThreadHandle, INFINITE);

    VirtualFreeEx(ulProcessHandle, pszRemoteDllPath, 0, MEM_RELEASE);
    CloseHandle(ulThreadHandle);
    CloseHandle(ulProcessHandle);

    return BS_OK;
}

static BS_STATUS _OS_INJDLL_InjectDll(IN UINT ulProcessId, IN CHAR *pszDllPath)
{
    CHAR *pszRemoteDllPath;
    UINT ulDllPathLen = strlen(pszDllPath) + 1;
    PTHREAD_START_ROUTINE pfnThreadRtn;
    HANDLE ulProcessHandle = NULL, ulThreadHandle = NULL;

    pfnThreadRtn = (PTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(TEXT("Kernel32")), "LoadLibraryA");
    if (pfnThreadRtn == NULL)
    {
        RETURN(BS_NO_SUCH);
    }

    ulProcessHandle = OpenProcess(
           PROCESS_QUERY_INFORMATION |   // Required by Alpha
           PROCESS_CREATE_THREAD     |   // For CreateRemoteThread
           PROCESS_VM_OPERATION      |   // For VirtualAllocEx/VirtualFreeEx
           PROCESS_VM_WRITE,             // For WriteProcessMemory
           FALSE, ulProcessId);
    if (ulProcessHandle == NULL)
    {
        RETURN(BS_CAN_NOT_OPEN);
    }

    /* 第一步: 申请远程进程内存 */
    pszRemoteDllPath = VirtualAllocEx(ulProcessHandle, NULL, ulDllPathLen, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (NULL == pszRemoteDllPath)
    {
        CloseHandle(ulProcessHandle);
        RETURN(BS_NO_MEMORY);
    }

    if (0 == WriteProcessMemory(ulProcessHandle, pszRemoteDllPath, pszDllPath, ulDllPathLen, NULL))
    {
        VirtualFreeEx(ulProcessHandle, pszRemoteDllPath, 0, MEM_RELEASE);
        CloseHandle(ulProcessHandle);
        RETURN(BS_ERR);
    }

    ulThreadHandle = CreateRemoteThread(ulProcessHandle, NULL, 0, pfnThreadRtn, pszRemoteDllPath, 0, NULL);
    if (ulThreadHandle == NULL)
    {
        VirtualFreeEx(ulProcessHandle, pszRemoteDllPath, 0, MEM_RELEASE);
        CloseHandle(ulProcessHandle);
        RETURN(BS_ERR);
    }

    /* Wait for the remote thread to terminate */
    WaitForSingleObject(ulThreadHandle, INFINITE);

    VirtualFreeEx(ulProcessHandle, pszRemoteDllPath, 0, MEM_RELEASE);
    CloseHandle(ulThreadHandle);
    CloseHandle(ulProcessHandle);

    return BS_OK;
}
#endif

#ifdef IN_UNIXLIKE
static BS_STATUS _OS_INJDLL_InjectDll(IN UINT ulProcessId, IN CHAR *pszDllPath)
{
    return BS_NOT_SUPPORT;
}
static BS_STATUS _OS_INJDLL_EjectDll(IN UINT ulProcessId, IN CHAR *pszDllPath)
{
    return BS_NOT_SUPPORT;
}
#endif

/* 注射DLL */
BS_STATUS INJDLL_InjectDll(IN UINT ulProcessId, IN CHAR *pszDllPath)
{
    return _OS_INJDLL_InjectDll(ulProcessId, pszDllPath);
}

/* 取消DLL */
BS_STATUS INJDLL_EjectDll(IN UINT ulProcessId, IN CHAR *pszDllPath)
{
    return _OS_INJDLL_EjectDll(ulProcessId, pszDllPath);
}

BS_STATUS INJDLL_CreateProcessAndInjectDll(IN CHAR *pszFileName, IN CHAR *pszDllPath)
{
	LONG lProcessID;
    
    lProcessID = PROCESS_CreateByFile(pszFileName, NULL, PROCESS_FLAG_SUSPEND | PROCESS_FLAG_HIDE);
	if (0 == lProcessID)
	{
		RETURN(BS_CAN_NOT_OPEN);
	}

	INJDLL_InjectDll(lProcessID, pszDllPath);

	PROCESS_Resume(lProcessID);

    return BS_OK;
}

