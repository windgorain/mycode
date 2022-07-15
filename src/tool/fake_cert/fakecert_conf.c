/*================================================================
*   Created by LiXingang: 2018.12.20
*   Description: 
*
================================================================*/
#include "bs.h"

#include "utl/cff_utl.h"
#include "utl/file_utl.h"

#include "fakecert_log.h"
#include "fakecert_conf.h"
#include "fakecert_service.h"

static char *g_fakecert_log_cfgfile = "./fakecert.conf";
static time_t g_fakecert_log_last_modify_time = 0;

int fakecert_conf_init()
{
    CFF_HANDLE hCff;
    time_t modify_time = 0;

    FILE_GetUtcTime(g_fakecert_log_cfgfile, NULL, &modify_time, NULL);

    if (modify_time == g_fakecert_log_last_modify_time) {
        return 0;
    }

    g_fakecert_log_last_modify_time = modify_time;

    hCff = CFF_INI_Open(g_fakecert_log_cfgfile, CFF_FLAG_READ_ONLY);
    if (NULL == hCff) {
        return -1;
    }

    fakecert_log_conf_init(hCff);
    fakecert_service_conf_init(hCff);

    CFF_Close(hCff);

    return 0;
}

