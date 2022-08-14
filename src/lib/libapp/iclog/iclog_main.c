/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-10-18
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
#include "utl/log_file.h"
#include "utl/txt_utl.h"
#include "utl/exec_utl.h"

static HANDLE g_iclog_file;
static UINT64 g_iclog_file_capacity;
static UINT64 g_iclog_events;
static IC_HANDLE g_iclog_handle;
static char g_iclog_logfile[256];

static VOID _iclog_log_output(IN CHAR *pcMsg, IN USER_HANDLE_S *pstUserHandle)
{
    LogFile_OutString(g_iclog_file, pcMsg);
    return;
}

static int iclog_enable()
{
    if (! g_iclog_file) {
        g_iclog_file = LogFile_Open(g_iclog_logfile);
        if (! g_iclog_file) {
            EXEC_OutString("Can't open iclog file");
            return -1;
        }
        LogFile_SetCapacity(g_iclog_file, g_iclog_file_capacity*1024);
        g_iclog_handle = IC_Reg(_iclog_log_output, NULL, g_iclog_events);
        if (! g_iclog_handle) {
            EXEC_OutString("Can't reg ic");
            LogFile_Close(g_iclog_file);
            g_iclog_file = NULL;
            return -1;
        }
    }

    return 0;
}

int iclog_disable()
{
    if (g_iclog_file) {
        LogFile_Close(g_iclog_file);
        IC_UnReg(g_iclog_handle);
        g_iclog_file = NULL;
        g_iclog_handle = NULL;
    }

    return 0;
}

/* file size %INT */
PLUG_API int ICLOG_SetLogFileSize(int argc, char **argv)
{
    g_iclog_file_capacity = TXT_Str2Ui(argv[2]);

    if (g_iclog_file) {
        LogFile_SetCapacity(g_iclog_file, g_iclog_file_capacity*1024);
    }

    return 0;
}

/* event {xxx} */
PLUG_API int ICLOG_SetEvents(int argc, char **argv)
{
    UINT events = IC_GetLogEvent(argv[1]);

    g_iclog_events |= events;

    if (g_iclog_handle) {
        IC_SetEvents(g_iclog_handle, g_iclog_events);
    }

    return 0;
}

/* no event {xxx} */
PLUG_API int ICLOG_NoEvents(int argc, char **argv)
{
    UINT events = IC_GetLogEvent(argv[2]);

    g_iclog_events &= (~events);

    if (g_iclog_handle) {
        IC_SetEvents(g_iclog_handle, g_iclog_events);
    }

    return 0;
}

/* [no] iclog enable */
PLUG_API int ICLOG_Enable(int argc, char **argv)
{
    if (argv[0][0] == 'n') {
        return iclog_disable();
    } 

    return iclog_enable();
}

static void iclog_SaveEvents(HANDLE hFile)
{
    char *strs[32];
    int count;
    int i;

    if (g_iclog_events == 0xffffffff) {
        CMD_EXP_OutputCmd(hFile, "event all");
        return;
    }

    count = IC_GetLogEventStr(g_iclog_events, strs);
    for (i=0; i<count; i++) {
        CMD_EXP_OutputCmd(hFile, "event %s", strs[i]);
    }
}

PLUG_API int ICLOG_Save(HANDLE hFile)
{
    iclog_SaveEvents(hFile);

    if (g_iclog_file_capacity > 0) {
        CMD_EXP_OutputCmd(hFile, "file size %llu", g_iclog_file_capacity);
    }

    if (g_iclog_file) {
        CMD_EXP_OutputCmd(hFile, "iclog enable");
    }

    return 0;
}

int ICLOG_Init()
{
    scnprintf(g_iclog_logfile, sizeof(g_iclog_logfile), "log/iclog/%s.txt", SYSINFO_GetSlefName());
    return 0;
}

