/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"

#include "utl/cff_utl.h"
#include "utl/txt_utl.h"
#include "../h/pwatcher_conf.h"

static int g_pwatcher_runner_num;
static int g_pwatcher_bucket_num;

static char g_pwatcher_conf_path[256];

int PWatcherConf_Init()
{
    CFF_HANDLE hCff;
    int var;

    SYSINFO_ExpandConfPath(g_pwatcher_conf_path, sizeof(g_pwatcher_conf_path), "plug/pwatcher/");

    hCff = PWatcherConf_Open("pwatcher_conf.ini");
    if (! hCff) {
        RETURN(BS_CAN_NOT_OPEN);
    }

    var = 0;
    CFF_GetPropAsInt(hCff, "runner", "runner", &var);
    g_pwatcher_runner_num = var;

    var = 1024;
    CFF_GetPropAsInt(hCff, "session", "bucket", &var);
    g_pwatcher_bucket_num = var;

    CFF_Close(hCff);

    return 0;
}

char * PWatcherConf_GetCfgPath()
{
    return g_pwatcher_conf_path;
}

int PWatcherConf_GetRunnerNum()
{
    return g_pwatcher_runner_num;
}

int PWatcherConf_GetSessBucketNum()
{
    return g_pwatcher_bucket_num;
}

char * PWatcherConf_BuildFullPath(char *tmp_path, OUT char *buf, int buf_size)
{
    scnprintf(buf, buf_size, "%s/%s", g_pwatcher_conf_path, tmp_path);

    return buf;
}

void * PWatcherConf_Open(char *ini_path)
{
    char buf[512];

    scnprintf(buf, sizeof(buf), "%s/%s", g_pwatcher_conf_path, ini_path);

    return CFF_INI_Open(buf, 0);
}

