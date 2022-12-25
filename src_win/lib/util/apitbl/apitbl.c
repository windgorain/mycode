/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-4-13
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#ifdef IN_WINDOWS
typedef int (WSAAPI *PF_CONNECT_FUNC)(IN int s, IN const struct sockaddr FAR * name, IN int namelen);
typedef int (WSAAPI *PF_WSAConnect_FUNC)(SOCKET s, const struct sockaddr FAR * name, int namelen, LPWSABUF lpCallerData, LPWSABUF lpCalleeData, LPQOS lpSQOS, LPQOS lpGQOS);
typedef UINT (WINAPI *PF_LoadLibraryA_FUNC)(IN CHAR * lpLibFileName);
typedef UINT (WINAPI *PF_LoadLibraryW_FUNC)(IN CHAR *pszFileName);
typedef UINT (WINAPI *PF_GetProcAddress_FUNC)(IN PLUG_HDL ulPlugId, IN CHAR *pszFuncName);
typedef UINT (WINAPI *PF_LoadLibraryExA_FUNC)(IN CHAR *pszFileName, IN HANDLE hFile, IN UINT dwFlags);
typedef HMODULE (WINAPI *PF_LoadLibraryExW_FUNC)(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);

typedef struct
{
    PF_CONNECT_FUNC pfConnect;
    PF_WSAConnect_FUNC pfWSAConnect;
    PF_LoadLibraryA_FUNC pfLoadLibraryA;
    PF_LoadLibraryW_FUNC pfLoadLibraryW;
    PF_LoadLibraryExA_FUNC pfLoadLibraryExA;
    PF_LoadLibraryExW_FUNC pfLoadLibraryExW;
    PF_GetProcAddress_FUNC pfGetProcAddress;
}_APITBL_FUNCS_S;

_APITBL_FUNCS_S g_stApiTbl = 
{
    (PF_CONNECT_FUNC)connect,
    (PF_WSAConnect_FUNC)WSAConnect,
    (PF_LoadLibraryA_FUNC)LoadLibraryA,
    (PF_LoadLibraryW_FUNC)LoadLibraryW,
    (PF_LoadLibraryExA_FUNC)LoadLibraryExA,
    (PF_LoadLibraryExW_FUNC)LoadLibraryExW,
    (PF_GetProcAddress_FUNC)GetProcAddress,
};

INT APITBL_Connect(IN int s, IN const struct sockaddr * name, IN int namelen)
{
    return g_stApiTbl.pfConnect(s, name, namelen);
}

INT APITBL_WSAConnect
(
    SOCKET s,
    const struct sockaddr FAR * name,
    int namelen,
    LPWSABUF lpCallerData,
    LPWSABUF lpCalleeData,
    LPQOS lpSQOS,
    LPQOS lpGQOS
)
{
    return g_stApiTbl.pfWSAConnect(s, name, namelen, lpCallerData, lpCalleeData, lpSQOS, lpGQOS);
}

UINT APITBL_LoadLibrary(IN CHAR *pszLibFileName)
{
    return g_stApiTbl.pfLoadLibraryA(pszLibFileName);
}

UINT APITBL_LoadLibraryW(IN CHAR *pszLibFileName)
{
    return g_stApiTbl.pfLoadLibraryW(pszLibFileName);
}

UINT APITBL_LoadLibraryExA(IN CHAR * lpLibFileName, IN UINT hFile, IN UINT dwFlags)
{
    return g_stApiTbl.pfLoadLibraryExA(lpLibFileName, (HANDLE)hFile, dwFlags);
}

UINT APITBL_LoadLibraryExW(IN CHAR * lpLibFileName, IN UINT hFile, IN UINT dwFlags)
{
    return (UINT)g_stApiTbl.pfLoadLibraryExW((LPCWSTR)((VOID*)lpLibFileName), (HANDLE)hFile, dwFlags);
}


VOID * APITBL_GetProcAddress(IN PLUG_HDL ulPlugId, IN CHAR *pszFuncName)
{
    return (VOID*) g_stApiTbl.pfGetProcAddress(ulPlugId, pszFuncName);
}

typedef struct
{
    CHAR *pszDllName;
    CHAR *pszFuncName;
    UINT *pulFuncPtr;
}_APITBL_PROTECT_FUNCS_S;

static _APITBL_PROTECT_FUNCS_S g_stApiTblProtectFuncs[]=
{
    {"ws2_32.dll", "connect", (UINT*)&g_stApiTbl.pfConnect},
    {"ws2_32.dll", "WSAConnect", (UINT*)&g_stApiTbl.pfWSAConnect},
    {"kernel32.dll", "LoadLibraryA", (UINT*)&g_stApiTbl.pfLoadLibraryA},
    {"kernel32.dll", "LoadLibraryW", (UINT*)&g_stApiTbl.pfLoadLibraryW},
    {"kernel32.dll", "LoadLibraryExA", (UINT*)&g_stApiTbl.pfLoadLibraryExA},
    {"kernel32.dll", "LoadLibraryExW", (UINT*)&g_stApiTbl.pfLoadLibraryExW},
    {"kernel32.dll", "GetProcAddress", (UINT*)&g_stApiTbl.pfGetProcAddress},
};

VOID _APITBL_InitProtectFunc()
{
    UINT pfFunc;
    PLUG_HDL ulPlugId;
    UINT i;

    for (i=0; i<sizeof(g_stApiTblProtectFuncs)/sizeof(_APITBL_PROTECT_FUNCS_S); i++)
    {
        ulPlugId = (PLUG_HDL)PLUG_LOAD(g_stApiTblProtectFuncs[i].pszDllName);
        if (0 == ulPlugId)
        {
            continue;
        }

        pfFunc = (UINT)PLUG_GET_FUNC_BY_NAME(ulPlugId, g_stApiTblProtectFuncs[i].pszFuncName);
        if (0 == pfFunc)
        {
            continue;
        }

        *g_stApiTblProtectFuncs[i].pulFuncPtr = pfFunc;
    }
}

CONSTRUCTOR(init) {
    _APITBL_InitProtectFunc();
}

#endif
