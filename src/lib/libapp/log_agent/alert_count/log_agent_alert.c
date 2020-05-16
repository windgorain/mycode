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
#include "utl/http_log.h"
#include "utl/match_utl.h"

#include "../h/log_agent_conf.h"
#include "../h/log_agent_alert.h"


static MATCH_HANDLE g_logagent_alert_match;

int LOGAGENT_ALERT_Init()
{
    g_logagent_alert_match = UintMatch_Create(LOGAGENT_ALERT_MACTCH_COUNT);
    return 0;
}

MATCH_HANDLE LOGAGENT_ALERT_GetCtrl()
{
    return g_logagent_alert_match;
}

PLUG_API void LOGAGENT_ALERT_Input(UINT sid)
{
    Match_Do(g_logagent_alert_match, &sid);
}


