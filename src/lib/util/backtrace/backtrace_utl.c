/*================================================================
*   Created by LiXingang: 2018.11.17
*   Description: 
*
================================================================*/
#include "bs.h"

#ifdef USE_BACKTRACE
#include <execinfo.h>
#endif

#ifdef IN_LINUX
#include <sys/prctl.h>
#endif

#define MAX_DEPTH 128

typedef void (*PF_BACKTRACE_PRINT)(char *info, void *ud);

static void _backtrace_call(PF_BACKTRACE_PRINT func, void *ud)
{
#ifdef USE_BACKTRACE
    void *buffer[MAX_DEPTH];
    int nptrs = backtrace(buffer, MAX_DEPTH);
    char **stack = backtrace_symbols(buffer, nptrs);
    int i;
    char info[256];
    
    if (stack) {
        for (i = 0; i < nptrs; ++i) {
            scnprintf(info, sizeof(info), "%p  %s \r\n", buffer[i], stack[i]);
            func(info, ud);
        }

        free(stack);
    } else {
        for (i = 0; i < nptrs; ++i) {
            scnprintf(info, sizeof(info), "%p \r\n", buffer[i]);
            func(info, ud);
        }
    }
#endif
}

static void _backtrace_print(char *info, void *ud)
{
    printf("%s", info);
}

static void _backtrace_write_fp(char *info, void *ud)
{
    FILE *fp = ud;
    fprintf(fp, "%s", info);
}

static void _backtrace_write_fd(char *info, void *ud)
{
    int fd = HANDLE_UINT(ud);
    write(fd, info, strlen(info));
}

void BackTrace_Print(void)
{
    _backtrace_call(_backtrace_print, NULL);
}

void BackTrace_WriteToFile(char *file)
{
    FILE *fp = fopen(file, "w+");
    if (!fp) {
        return;
    }

    _backtrace_call(_backtrace_write_fp, fp);

    fclose(fp);
}

void BackTrace_WriteToFp(FILE *fp)
{
    _backtrace_call(_backtrace_write_fp, fp);
}

void BackTrace_WriteToFd(int fd)
{
    _backtrace_call(_backtrace_write_fd, UINT_HANDLE(fd));
}

void CoreDump_Enable(void)
{
#ifdef IN_LINUX
    prctl(PR_SET_DUMPABLE, 1);
#endif
    return;
}

