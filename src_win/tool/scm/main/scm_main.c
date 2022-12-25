/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-11-30
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
#include "utl/sys_utl.h"

static PLUG_HDL g_hPlug;
static VOID_FUNC g_scm_stop_func = NULL;

static BOOL_T scm_ConsoleCtrlHander(IN INT lEvent)
{
    VOID_FUNC pfFunc;
    
    switch (lEvent)
     {
        // Handle the CTRL+C signal.
        case CTRL_C_EVENT:
        case CTRL_SHUTDOWN_EVENT:
        case CTRL_CLOSE_EVENT:
        {
            pfFunc = PLUG_GET_FUNC_BY_NAME(g_hPlug, "PlugStop");
            if (pfFunc != NULL)
            {
                pfFunc();
            }
            break;
        }

        // Pass other signals to the next handler.
        case CTRL_LOGOFF_EVENT:
        case CTRL_BREAK_EVENT:
        default:
        {
            break;
        }
     }

    return FALSE;
}

static void scm_start()
{
    UINT_FUNC_2 pfFunc;

    g_hPlug= PLUG_LOAD("scm.dll");
    if (g_hPlug == NULL)
    {
        printf("Can't load scm.dll");
        return -1;
    }

    pfFunc = (UINT_FUNC_2)PLUG_GET_FUNC_BY_NAME(g_hPlug, "PlugRun");
    if (pfFunc == NULL)
    {
        PLUG_FREE(g_hPlug);
        printf("Can't get function PlugRun");
        return -1;
    }

    g_scm_stop_func = (VOID_FUNC)PLUG_GET_FUNC_BY_NAME(g_hPlug, "PlugStop");

    SetConsoleCtrlHandler((PHANDLER_ROUTINE)scm_ConsoleCtrlHander, TRUE);

    return pfFunc((VOID*)argc, argv);
}

static void scm_stop()
{
    if (g_scm_stop_func) {
        g_scm_stop_func();
    }
}

int main(int argc, char **argv)
{
    char *cmd = "";

    if (argc >= 2) {
        cmd = argv[1];
    }

    WinService_Init("scm", scm_start, scm_stop);

    if (cmd[0] == 'i') {
        WinService_Install(SYS_GetSelfFileName());
    } else if (cmd[0] == 'u') {
        WinService_Uninstall();
    } else if (cmd[0] == '\0') {
        WinService_Run();
    } else {
        printf("Usage: %s [install/unstall]\r\n", argv[0]);
    }

    return 0;
}
