/*================================================================
*   Created：2020.02.26
*   Description：
*
================================================================*/
#include "bs.h"
#include <syslog.h>
#include "utl/syslog_utl.h"

int SYSLOG(int level, const char *fmt, ...)
{
    char buf[LOG_MSG_SZ];
    char log_str[LOG_MSG_SZ];
    va_list args;
    time_t t;
    struct tm tm;

    va_start(args, fmt);
    vsnprintf(buf, LOG_MSG_SZ, fmt, args);
    va_end(args);

    t = time(NULL);
    localtime_r((const time_t *)&t, &tm);

    snprintf(log_str, LOG_MSG_SZ - 1, "%04d-%02d-%02d %02d:%02d:%02d %s",
                    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, buf);

    syslog(level, "%s", log_str);
    
    return 0;
}
