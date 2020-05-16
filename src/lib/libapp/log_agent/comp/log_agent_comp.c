/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/match_utl.h"
#include "comp/comp_logagent.h"
#include "../h/log_agent_http.h"
#include "../h/log_agent_alert.h"

static COMP_LOGAGENT_S g_logagent_comp = {
    .comp.comp_name = COMP_LOGAGENT_NAME,

    .set_conf_dir = LOGAGENT_CONF_SetConfDir,
    .set_log_dir = LOGAGENT_CONF_SetLogDir,

    .http_parse_config = LOGAGENT_HTTP_ParseConfig,
    .http_head_input = LOGAGENT_HTTP_HeadInput,
    .http_body_input = LOGAGENT_HTTP_BodyInput,

    .tcp_parse_config = LOGAGENT_TCP_ParseConfig,
    .tcp_input = LOGAGENT_TCP_Input,

    .alert_input = LOGAGENT_ALERT_Input
};

int LOGAGENT_COMP_Init()
{
    COMP_Reg(&g_logagent_comp.comp);
    return 0;
}
