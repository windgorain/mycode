/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-4-13
* Description: 
* History:     
******************************************************************************/

#ifndef __APITBL_H_
#define __APITBL_H_

#ifdef __cplusplus
    extern "C" {
#endif 

#ifdef IN_WINDOWS

PLUG_API INT APITBL_Connect(IN int s, IN const struct sockaddr * name, IN int namelen);
PLUG_API INT APITBL_WSAConnect
(
    SOCKET s,
    const struct sockaddr FAR * name,
    int namelen,
    LPWSABUF lpCallerData,
    LPWSABUF lpCalleeData,
    LPQOS lpSQOS,
    LPQOS lpGQOS
);
PLUG_API UINT APITBL_LoadLibrary(IN CHAR *pszLibFileName);
PLUG_API VOID * APITBL_GetProcAddress(IN PLUG_HDL ulPlugId, IN CHAR *pszFuncName);
PLUG_API UINT APITBL_LoadLibraryW(IN CHAR *pszLibFileName);
PLUG_API UINT APITBL_LoadLibraryExA(IN CHAR * lpLibFileName, IN UINT hFile, IN UINT dwFlags);
PLUG_API UINT APITBL_LoadLibraryExW(IN CHAR * lpLibFileName, IN UINT hFile, IN UINT dwFlags);

#endif

#ifdef __cplusplus
    }
#endif 

#endif 


