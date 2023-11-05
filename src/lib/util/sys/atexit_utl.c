/*================================================================
*   Created by LiXingang: 2019.01.04
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/signal_utl.h"
#include "utl/atexit_utl.h"

#ifdef IN_WINDOWS
static BOOL_T _win_abort_notify(IN INT lEvent)
{
    switch (lEvent)
     {
        
        case CTRL_C_EVENT:
        case CTRL_SHUTDOWN_EVENT:
        case CTRL_CLOSE_EVENT:
        {
            exit(0);
            return FALSE;
        }

        
        case CTRL_LOGOFF_EVENT:
        case CTRL_BREAK_EVENT:
        default:
            return FALSE;
     }
}

static int _os_abort_watcher()
{
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)_win_abort_notify, TRUE);
	return 0;
}
#endif

#ifdef IN_UNIXLIKE
static VOID _unix_abort_notify(IN int iSigno)
{
    exit(0);
}

static int _os_abort_watcher()
{
    SIGNAL_Set(SIGINT, 0, _unix_abort_notify);
	return 0;
}
#endif

int ATEXIT_Reg(VOID_FUNC func)
{
    _os_abort_watcher();
    atexit(func);
    return 0;
}

