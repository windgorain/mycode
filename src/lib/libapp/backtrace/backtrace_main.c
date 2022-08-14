/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-10-18
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
#include "utl/backtrace_utl.h"
#include "utl/signal_utl.h"
#include "comp/comp_backtrace.h"

#ifdef IN_UNIXLIKE

#define BACKTRACE_NOTIFY_NODES_MAX 32

static PF_BACKTRACE_NOTIFY g_backtrace_notify_nodes[BACKTRACE_NOTIFY_NODES_MAX];

static void _backtrace_print(int sig)
{
    printf("sig=%d \r\n\r\n", sig);
    BackTrace_Print();
}

static void _backtrace_write_file(int sig)
{
    FILE *fp = fopen("backtrace/backtrace.txt", "w+");
    if (!fp) {
        return;
    }

    fprintf(fp, "sig=%d \r\n\r\n", sig);
    BackTrace_WriteToFp(fp);

    fclose(fp);
}

static void _backtrace_notify(int sig)
{
    int i;

    for (i=0; i<BACKTRACE_NOTIFY_NODES_MAX; i++) {
        if (g_backtrace_notify_nodes[i]) {
            g_backtrace_notify_nodes[i](sig);
        }
    }
}

static void _btrace_signal(int sig)
{
    _backtrace_print(sig);
    _backtrace_write_file(sig);
    _backtrace_notify(sig);
    signal(sig, SIG_DFL); /* 恢复信号默认处理 */  
    raise(sig);  /* 再次发送信号 */
}

#endif

int BACKTRACE_RegWatcher(PF_BACKTRACE_NOTIFY watcher_func)
{
    int i;

    for (i=0; i<BACKTRACE_NOTIFY_NODES_MAX; i++) {
        if (! g_backtrace_notify_nodes[i]) {
            g_backtrace_notify_nodes[i] = watcher_func;
            return 0;
        }
    }

    RETURN(BS_NO_RESOURCE);
}

int BACKTRACE_Init()
{
#ifdef IN_UNIXLIKE
    SIGNAL_Set(SIGILL, FALSE, _btrace_signal);
    SIGNAL_Set(SIGBUS, FALSE, _btrace_signal);
    SIGNAL_Set(SIGFPE, FALSE, _btrace_signal);
    SIGNAL_Set(SIGSEGV, FALSE, _btrace_signal);
    SIGNAL_Set(SIGSYS, FALSE, _btrace_signal);
#endif

    return 0;
}

