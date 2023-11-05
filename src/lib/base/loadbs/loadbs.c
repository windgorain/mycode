/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-3-17
* Description: 用于给使用bs.dll的应用程序调用,以加载bs.dll
* History:     
******************************************************************************/
#include "bs.h"
#include "bs/loadbs.h"
#include "bs/loadcmd.h"
#include "../h/secinit_bs.h"

#include "utl/sys_utl.h"
#include "utl/file_utl.h"

#ifdef IN_UNIXLIKE
#include "utl/signal_utl.h"
#endif


#ifdef IN_WINDOWS
static BOOL_T _Load_ConsoleCtrlHander(IN INT lEvent)
{
    switch (lEvent)
     {
        
        case CTRL_C_EVENT:
        case CTRL_SHUTDOWN_EVENT:
        case CTRL_CLOSE_EVENT:
        {
            SYSRUN_Exit(0);
            return FALSE;
        }

        
        case CTRL_LOGOFF_EVENT:
        case CTRL_BREAK_EVENT:
        default:
            return FALSE;
     }
}

static BS_STATUS _Load_RegConsoleCtrlHander()
{
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)_Load_ConsoleCtrlHander, TRUE);
	return BS_OK;
}
#endif

#ifdef IN_UNIXLIKE
static BS_STATUS _Load_RegConsoleCtrlHander()
{
    

	return BS_OK;
}

#if 0 
static void loadbs_linuxDump(int signo)
{
    char buf[1024];
    char cmd[1024];
    FILE *fh;

    Load_Cmd_Fini();

    scnprintf(buf, sizeof(buf), "/proc/%d/cmdline", getpid());
    if(!(fh = fopen(buf, "r")))
            exit(0);
    if(!fgets(buf, sizeof(buf), fh))
            exit(0);
    fclose(fh);
    if(buf[strlen(buf) - 1] == '\n')
            buf[strlen(buf) - 1] = '\0';
    scnprintf(cmd, sizeof(cmd), "gdb %s %d", buf, getpid());
    system(cmd);

    exit(0);
}
#endif
#endif

PLUG_API void LoadBs_SetArgv(IN UINT uiArgc, IN CHAR **ppcArgv)
{
    SYSINFO_SetArgv(uiArgc, ppcArgv);
}

PLUG_API INT LoadBs_Init()
{
    static BOOL_T bIsInit = FALSE;

    if (bIsInit == TRUE)
    {
        return 0;
    }

    bIsInit = TRUE;

    FILE_ChangeCurrentDir(SYS_GetSelfFilePath());

#ifdef IN_UNIXLIKE
    SIGNAL_Set(SIGPIPE, 0, (VOID *)SIG_IGN);
#endif

    _Load_RegConsoleCtrlHander();

    if (BS_OK != SECINIT_Init())
    {
        return -1;
    }

    return 0;
}

void LoadBs_SetMainMode()
{
    PollerBs_SetMainMode();
}

void LoadBs_Main()
{
    PollerBs_Run();
}

