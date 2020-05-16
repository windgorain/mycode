/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"

#include "../h/log_agent_conf.h"

static char *logagent_conf_dir = "";
static char *logagent_log_dir = "";

PLUG_API void LOGAGENT_CONF_SetConfDir(char *conf_dir)
{
    logagent_conf_dir = conf_dir;
}

PLUG_API void LOGAGENT_CONF_SetLogDir(char *log_dir)
{
    logagent_log_dir = log_dir;
}

char * LOGAGENT_CONF_GetConfDir()
{
    return logagent_conf_dir;
}

char * LOGAGENT_CONF_GetLogDir()
{
    return logagent_log_dir;
}

