/*================================================================
*   Created：2020.02.26
*   Description：
*
================================================================*/
#ifndef _SYSLOG_UTL_H
#define _SYSLOG_UTL_H
#include <syslog.h>
#ifdef __cplusplus
extern "C"
{
#endif

#define LOG_MSG_SZ       1024

extern int SYSLOG(int level, char *type, const char *fmt, ...);

#ifdef __cplusplus
}
#endif
#endif //SYSLOG_UTL_H_
