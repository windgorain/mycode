/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-11-20
* Description: 
* History:     
******************************************************************************/

#ifndef __WIN_NATIVE_API_H_
#define __WIN_NATIVE_API_H_

#ifdef __cplusplus
    extern "C" {
#endif 

#ifdef IN_WINDOWS


typedef struct proc_basic_info
{
    UINT32 ExitStatus; 
    UINT32 PebBaseAddress; 
    UINT32 AffinityMask; 
    UINT32 BasePriority; 
    UINT32 UniqueProcessId; 
    UINT32 InheritedFromUniqueProcessId; 
}PROCESS_BASIC_INFORAMTION;

UINT32 ZwQueryInformationProcess
(
    IN HANDLE hProcess,
    IN UINT32 ulClass,
    OUT PROCESS_BASIC_INFORAMTION *pstInfo,
    IN UINT32 ulInfoSize,
    OUT UINT32 *pulCopySize  
);


typedef struct thread_basic_info
{
    UINT32 ExitStatus; 
    UINT32 TebBaseAddress; 
    UINT32 ClientId; 
    UINT32 AffinityMask; 
    UINT32 BasePriority; 
    UINT32 Priority; 
}THREAD_BASIC_INFORMATION;

UINT32 ZwQueryInformationThread
(
    IN HANDLE hThrad,
    IN UINT32 ulClass,
    OUT THREAD_BASIC_INFORMATION *pstInfo,
    IN UINT32 ulInfoSize,
    OUT UINT32 *pulCopySize  
);



typedef struct
{
    NT_TIB Tib;
    PVOID EnvironmentPointer;
    UINT32 Cid;
    PVOID ActiveRpcInfo;
    PVOID ThreadLocalStoragePointer;
    VOID  *Peb; 
    UINT32 LastErrorValue;
    UINT32 CountOfOwnedCriticalSections;
    PVOID CsrClientThread;
    PVOID Win32ThreadInfo;
    UINT32 Win32ClientInfo[0x1F];
    PVOID WOW32Reserved;
    UINT32 CurrentLocale;
    UINT32 FpSoftwareStatusRegister;
    PVOID SystemReserved1[0x36];
    PVOID Spare1;
    UINT32 ExceptionCode; 
    UINT32 SpareBytes1[0x28];
    PVOID SystemReserved2[0xA];
    UINT32 GdiRgn;
    UINT32 GdiPen;
    UINT32 GdiBrush;
    UINT32 RealClientId;
    PVOID GdiCachedProcessHandle;
    UINT32 GdiClientPID;
    UINT32 GdiClientTID;
    PVOID GdiThreadLocaleInfo;
    PVOID UserReserved[5];
    PVOID GlDispatchTable[0x118];
    UINT32 GlReserved1[0x1A];
    PVOID GlReserved2;
    PVOID GlSectionInfo;
    PVOID GlSection;
    PVOID GlTable;
    PVOID GlCurrentRC;
    PVOID GlContext;
    UINT32 LastStatusValue;
    CHAR  *StaticUnicodeString;
    WCHAR StaticUnicodeBuffer[0x105];
    PVOID DeallocationStack;
    PVOID TlsSlots[0x40];
    LIST_ENTRY TlsLinks;
    PVOID Vdm;
    PVOID ReservedForNtRpc;
    PVOID DbgSsReserved[0x2];
    UINT32 HardErrorDisabled;
    PVOID Instrumentation[0x10];
    PVOID WinSockData;
    UINT32 GdiBatchCount;
    UINT32 Spare2;
    UINT32 Spare3;
    UINT32 Spare4;
    PVOID ReservedForOle;
    UINT32 WaitingOnLoaderLock;
    PVOID StackCommit;
    PVOID StackCommitMax;
    PVOID StackReserved;
} TEB_S;


#endif

#ifdef __cplusplus
    }
#endif 

#endif 


