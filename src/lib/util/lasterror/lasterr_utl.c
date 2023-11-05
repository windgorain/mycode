/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-8-27
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/vnic_lib.h"
#include "utl/txt_utl.h"

#ifdef IN_WINDOWS
static VOID _lasterr_Print()
{
    LPVOID lpMsgBuf;
    
    FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM | 
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        0, 
        (LPTSTR) &lpMsgBuf,
        0,
        NULL 
    );

    printf(" %s\r\n", lpMsgBuf);

    LocalFree(lpMsgBuf);
}

static VOID _lasterr_SPrint(OUT CHAR *pcInfo)
{
    LPVOID lpMsgBuf;
    
    FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM | 
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        0, 
        (LPTSTR) &lpMsgBuf,
        0,
        NULL 
    );

    sprintf(pcInfo, " %s\r\n", lpMsgBuf);

    LocalFree(lpMsgBuf);
}

static VOID _lasterr_PrintByErrno(IN UINT uiErrno)
{
    LPVOID lpMsgBuf;
    
    FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM | 
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        uiErrno,
        0, 
        (LPTSTR) &lpMsgBuf,
        0,
        NULL 
    );

    printf(" %s\r\n", lpMsgBuf);

    LocalFree(lpMsgBuf);
}

#endif

#ifdef IN_UNIXLIKE
static VOID _lasterr_Print()
{
    return;
}
static VOID _lasterr_SPrint(OUT CHAR *pcInfo)
{
    return;
}
static VOID _lasterr_PrintByErrno(IN UINT uiErrno)
{
    return;
}
#endif

VOID LastErr_Print()
{
    _lasterr_Print();
}

VOID LastErr_SPrint(OUT CHAR *pcInfo)
{
    _lasterr_SPrint(pcInfo);
}

VOID LastErr_PrintByErrno(IN UINT uiErrno)
{
    _lasterr_PrintByErrno(uiErrno);
}

