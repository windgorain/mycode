/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2011-7-6
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/signal_utl.h"

#ifdef IN_UNIXLIKE
VOID * signal_Set(IN int iSigno, IN BOOL_T bRestart/*是否重启*/, IN VOID * pfFunc)
{
    struct sigaction act, oact;

    act.sa_handler = pfFunc;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    if (iSigno == SIGALRM) {
        bRestart = FALSE;
    }

    if (bRestart) {
#ifdef SA_RESTART
        act.sa_flags |= SA_RESTART;
#endif
    } else {
#ifdef SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT;
#endif
    }

    if (sigaction(iSigno, &act, &oact) < 0)
    {
        return SIG_ERR;
    }

    return oact.sa_handler;
}
#endif

#ifdef IN_WINDOWS
VOID * signal_Set(IN int iSigno, IN BOOL_T bRestart/*是否重启*/, IN VOID * pfFunc)
{
    return signal(iSigno, pfFunc);
}
#endif

VOID * SIGNAL_Set(IN int iSigno, IN BOOL_T bRestart/*是否重启*/, IN VOID * pfFunc)
{
    return signal_Set(iSigno, bRestart, pfFunc);
}
