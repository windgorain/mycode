/*================================================================
*   Created by LiXingang: 2018.11.28
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/txt_utl.h"
#include "utl/lstr_utl.h"
#include "utl/cff_utl.h"
#include "utl/file_utl.h"
#include "utl/log_utl.h"

#include "fakecert_log.h"

UINT g_fakecert_log_flag = 0;

static LOG_UTL_S g_fakecert_log;

int fakecert_log_conf_init(IN CFF_HANDLE hCff)
{
    UINT value;
    char *string;

    g_fakecert_log_flag = 0;
    LOG_Fini(&g_fakecert_log);

    value = 0;
    CFF_GetPropAsUint(hCff, "log", "process", &value);
    if (value) {
        g_fakecert_log_flag |= FAKECERT_LOG_FLAG_PROCESS;
    }

    value = 0;
    CFF_GetPropAsUint(hCff, "log", "event", &value);
    if (value) {
        g_fakecert_log_flag |= FAKECERT_LOG_FLAG_EVENT;
    }

    value = 0;
    CFF_GetPropAsUint(hCff, "log", "error", &value);
    if (value) {
        g_fakecert_log_flag |= FAKECERT_LOG_FLAG_ERROR;
    }

    LOG_Init(&g_fakecert_log);

    string = NULL;
    CFF_GetPropAsString(hCff, "log", "file", &string);
    if (string != NULL) {
        LOG_SetFileName(&g_fakecert_log, string);
    }

    value = 0;
    CFF_GetPropAsUint(hCff, "log", "print", &value);
    LOG_SetPrint(&g_fakecert_log, value);

    return 0;
}

void fakecert_log(IN CHAR *fmt, ...)
{
    va_list args;
    char msg[1024];

    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    LOG_Output(&g_fakecert_log, msg, strlen(msg));
}

