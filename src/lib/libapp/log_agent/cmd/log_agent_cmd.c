/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/log_utl.h"
#include "utl/socket_utl.h"
#include "utl/txt_utl.h"
#include "utl/exec_utl.h"
#include "utl/match_utl.h"
#include "utl/tcp_log.h"
#include "utl/http_log.h"

#include "../h/log_agent_tcp.h"
#include "../h/log_agent_http.h"
#include "../h/log_agent_alert.h"
#include "../h/log_agent_conf.h"


PLUG_API int LOGAGENT_CmdShowConfigPath(int argc, char **argv)
{
    EXEC_OutInfo("Config path: %s\r\n", LOGAGENT_CONF_GetConfDir());
    EXEC_OutInfo("Log path: %s\r\n", LOGAGENT_CONF_GetLogDir());
    return 0;
}

PLUG_API int LOGAGENT_Save(HANDLE hFile)
{
    LOGAGENT_TCP_Save(hFile);
    LOGAGENT_HTTP_Save(hFile);
    LOGAGENT_ALERT_Save(hFile);

    return 0;
}

