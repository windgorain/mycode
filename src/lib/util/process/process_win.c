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



DWORD WIN_GetProcessIDFromName(IN LPCSTR szName)
{
    DWORD id = 0; 
    PROCESSENTRY32 pe; 
    HANDLE hSnapshot;

    pe.dwSize = sizeof(PROCESSENTRY32);    
    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); 
    
    if(Process32First(hSnapshot, &pe)) 
    {
        do {
            if(0 == _stricmp(pe.szExeFile, szName)) 
            {
                id = pe.th32ProcessID;
                break;
            }
        }while(Process32Next(hSnapshot, &pe)); 
    }

    CloseHandle(hSnapshot); 

    return id;
}



DWORD WIN_GetMainThreadIdFromProcessId(IN DWORD idProcess)
{
    DWORD idThread = 0;
    HANDLE hSnapshot;
    THREADENTRY32 te;

    te.dwSize = sizeof(THREADENTRY32);

    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0); 
    if(Thread32First(hSnapshot, &te)) 
    {
        do {
            if(idProcess == te.th32OwnerProcessID) 
            {
                idThread = te.th32ThreadID;
                break;
            }
        }while(Thread32Next(hSnapshot, &te)); 
    }

    CloseHandle(hSnapshot); 

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

    
    if( !CreateProcess(NULL,   
        (LPTSTR)szParam, 
        NULL,             
        NULL,             
        FALSE,            
        ulFlag,                
        NULL,             
        NULL,             
        &si,              
        &pi )             
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


