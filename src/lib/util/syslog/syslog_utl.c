/*================================================================
*   Created：2020.02.26
*   Description：
*
================================================================*/
#include "bs.h"
#include <syslog.h>
#include "utl/syslog_utl.h"

int SYSLOG(int level, char* type, const char *fmt, ...)
{
    char buf[LOG_MSG_SZ];
    char log_str[LOG_MSG_SZ];
    va_list args;

	memset(log_str, 0, sizeof(log_str));
    va_start(args, fmt);
    vsnprintf(buf, LOG_MSG_SZ, fmt, args);
    va_end(args);

	if(type) {
    	syslog(level, "{\"type\":\"%s\",%s}",type, buf);
	}else {
		syslog(level, "%s", buf);
	}
    return 0;
}
