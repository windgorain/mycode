/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"

#include "../h/log_agent_conf.h"

static char *g_logagent_conf_dir = "";
static char *g_logagent_log_dir = "";

PLUG_API void LOGAGENT_CONF_SetConfDir(char *conf_dir)
{
    g_logagent_conf_dir = conf_dir;
}

PLUG_API void LOGAGENT_CONF_SetLogDir(char *log_dir)
{
    g_logagent_log_dir = log_dir;
}

char * LOGAGENT_CONF_GetConfDir()
{
    return g_logagent_conf_dir;
}

char * LOGAGENT_CONF_GetLogDir()
{
    return g_logagent_log_dir;
}
