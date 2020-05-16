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
static char g_precver_conf_path[256] = "plug/precver/";

int PRecver_Conf_SetCfgPath(char *cfgpath)
{
    int len;

    if (! cfgpath) {
        return 0;
    }

    /* 去除前后空白字符 */
    cfgpath = TXT_Strim(cfgpath);
    len = strlen(cfgpath);
    if (len == 0)  {
        return 0;
    }

    if (len >= sizeof(g_precver_conf_path)) {
        RETURN(BS_TOO_LONG);
    }

    strlcpy(g_precver_conf_path, cfgpath, sizeof(g_precver_conf_path));

    return 0;
}

int PRecver_Conf_Init()
{
    CFF_HANDLE hCff;
    int var;

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
    snprintf(buf, buf_size, "%s/%s", g_precver_conf_path, tmp_path);

    return buf;
}

void * PRecver_Conf_Open(char *ini_path)
{
    char buf[512];

    snprintf(buf, sizeof(buf), "%s/%s", g_precver_conf_path, ini_path);

    return CFF_INI_Open(buf, 0);
}

