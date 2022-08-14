/******************************************************************************
* Copyright (C),    LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2014-3-25
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#ifdef IN_WINDOWS
#include "utl/process_utl.h"

#include "process_inc.h"
#include "tlhelp32.h"

/* 提升进程权限 */
BOOL WIN_EnableDebugPrivilege(){
    HANDLE hToken;
    static BOOL fOk = FALSE;

    if (fOk)
    {
        return fOk;
    }

    if(OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES,&hToken)){
        TOKEN_PRIVILEGES tp;
        tp.PrivilegeCount=1;
        LookupPrivilegeValue(NULL,SE_DEBUG_NAME,&tp.Privileges[0].Luid);

        tp.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
        AdjustTokenPrivileges(hToken,FALSE,&tp,sizeof(tp),NULL,NULL);

        fOk=(GetLastError()==ERROR_SUCCESS);
        CloseHandle(hToken);
    }

    return fOk;
}

// 由进程名获取进程ID
// 失败返回0
DWORD WIN_GetProcessIDFromName(IN LPCSTR szName)
{
    DWORD id = 0; // 进程ID
    PROCESSENTRY32 pe; // 进程信息 
    HANDLE hSnapshot;

    pe.dwSize = sizeof(PROCESSENTRY32);    
    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); // 获取系统进程列表 
    
    if(Process32First(hSnapshot, &pe)) // 返回系统中第一个进程的信息 
    {
        do {
            if(0 == _stricmp(pe.szExeFile, szName)) // 不区分大小写比较
            {
                id = pe.th32ProcessID;
                break;
            }
        }while(Process32Next(hSnapshot, &pe)); // 下一个进程
    }

    CloseHandle(hSnapshot); // 删除快照

    return id;
}

// 由进程名获取主线程ID(需要头文件tlhelp32.h)
// 失败返回0
DWORD WIN_GetMainThreadIdFromProcessId(IN DWORD idProcess)
{
    DWORD idThread = 0;
    HANDLE hSnapshot;
    THREADENTRY32 te;

    te.dwSize = sizeof(THREADENTRY32);

    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0); // 系统所有线程快照
    if(Thread32First(hSnapshot, &te)) // 第一个线程 
    {
        do {
            if(idProcess == te.th32OwnerProcessID) // 认为找到的第一个该进程的线程为主线程 
            {
                idThread = te.th32ThreadID;
                break;
            }
        }while(Thread32Next(hSnapshot, &te)); // 下一个线程
    }

    CloseHandle(hSnapshot); // 删除快照

    return idThread;
} 


LONG _OS_PROCESS_CreateByFile
(
    IN CHAR *pcFilePath,
    IN CHAR *pcParam,
    IN UINT uiFlag
)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    UINT ulFlag = 0;
    CHAR szParam[1024];

    ZeroMemory( &si, sizeof(si) );
    ZeroMemory( &pi, sizeof(pi) );

    if (pcParam == NULL)
    {
        pcParam = "";
    }

    scnprintf(szParam, sizeof(szParam), "%s %s", pcFilePath, pcParam);

    if (uiFlag & PROCESS_FLAG_SUSPEND)
    {
        ulFlag |= CREATE_SUSPENDED;
    }

    if (uiFlag & PROCESS_FLAG_HIDE)
    {
        ulFlag |= CREATE_NO_WINDOW;
        si.wShowWindow = SW_HIDE;
        si.dwFlags = STARTF_USESHOWWINDOW;
    }

    si.cb = sizeof(si);

    // Start the child process. 
    if( !CreateProcess(NULL,   
        (LPTSTR)szParam, 
        NULL,             // Process handle not inheritable. 
        NULL,             // Thread handle not inheritable. 
        FALSE,            // Set handle inheritance to FALSE. 
        ulFlag,                // No creation flags. 
        NULL,             // Use parent's environment block. 
        NULL,             // Use parent's starting directory. 
        &si,              // Pointer to STARTUPINFO structure.
        &pi )             // Pointer to PROCESS_INFORMATION structure.
    )
    {
        return 0;
    }

    if (uiFlag & PROCESS_FLAG_WAIT_FOR_OVER)
    {
        WaitForSingleObject(pi.hProcess, INFINITE);
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    
    return pi.dwProcessId;
}

BS_STATUS _OS_PROCESS_Resume(IN LONG lProcessID)
{
    HANDLE ulThreadHandle;
    DWORD dwThreadID;

    dwThreadID = WIN_GetMainThreadIdFromProcessId(lProcessID);
    if (0 == dwThreadID)
    {
        return BS_ERR;
    }

    ulThreadHandle = OpenThread(THREAD_SUSPEND_RESUME, FALSE, dwThreadID);
    if (ulThreadHandle == 0)
    {
        return (BS_CAN_NOT_OPEN);
    }
    
    ResumeThread(ulThreadHandle);
    CloseHandle(ulThreadHandle);

    return BS_OK;
}

BS_STATUS _OS_PROCESS_SuspendMainThread(IN LONG lProcessID)
{
    HANDLE ulThreadHandle;
    DWORD dwThreadID;

    dwThreadID = WIN_GetMainThreadIdFromProcessId(lProcessID);
    if (0 == dwThreadID)
    {
        return BS_ERR;
    }

    ulThreadHandle = OpenThread(THREAD_SUSPEND_RESUME, FALSE, dwThreadID);
    if (ulThreadHandle == 0)
    {
        return (BS_CAN_NOT_OPEN);
    }
    
    SuspendThread(ulThreadHandle);
    CloseHandle(ulThreadHandle);

    return BS_OK;
}

HANDLE _OS_PROCESS_GetProcess(IN LONG lProcessID)
{
    WIN_EnableDebugPrivilege();
    return OpenProcess(PROCESS_ALL_ACCESS, FALSE, lProcessID);
}

BOOL_T _OS_PROCESS_IsPidExist(IN UINT pid)
{
    BS_DBGASSERT(0);
    return FALSE;
}

BOOL_T _OS_PROCESS_IsProcessNameExist(IN char *process)
{
    BS_DBGASSERT(0);
    return FALSE;
}

int _OS_PROCESS_RenameSelf(IN char *new_name)
{
    RETURN(BS_NOT_SUPPORT);
}

UINT _OS_PROCESS_GetPid()
{
    return GetCurrentProcessId();
}

UINT64 _OS_PROCESS_GetTid()
{
    return GetCurrentThreadId();
}

#endif


