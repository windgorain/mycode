/*================================================================
*   Created by LiXingang
*   Description: 
*
===============================================================*/
#include "bs.h"

#include "utl/cff_utl.h"
#include "utl/txt_utl.h"
#include "../h/precver_conf.h"

static int g_precver_worker_num;
static char g_precver_conf_path[256];

int PRecver_Conf_Init()
{
    CFF_HANDLE hCff;
    int var;

    SYSINFO_ExpandConfPath(g_precver_conf_path, sizeof(g_precver_conf_path), "plug/precver/");

    hCff = PRecver_Conf_Open("precver_conf.ini");
    if (! hCff) {
        RETURN(BS_CAN_NOT_OPEN);
    }

    var = 0;
    CFF_GetPropAsInt(hCff, "worker", "number", &var);
    g_precver_worker_num = var;

    CFF_Close(hCff);

    return 0;
}

int PRecver_Conf_GetWorkerNum()
{
    return g_precver_worker_num;
}

char *PRecver_Conf_GetCfgPath()
{
    return g_precver_conf_path;
}

char * PRecver_Conf_BuildFullPath(char *tmp_path, OUT char *buf, int buf_size)
{
    scnprintf(buf, buf_size, "%s/%s", g_precver_conf_path, tmp_path);

    return buf;
}

void * PRecver_Conf_Open(char *ini_path)
{
    char buf[512];

    scnprintf(buf, sizeof(buf), "%s/%s", g_precver_conf_path, ini_path);

    return CFF_INI_Open(buf, 0);
}

