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
#include "utl/http_log.h"

#include "../h/log_agent_alert.h"
#include "../h/log_agent_conf.h"


PLUG_API int LOGAGENT_ALERT_CmdMatchSid(int argc, char **argv)
{
    MATCH_HANDLE hMatch = LOGAGENT_ALERT_GetCtrl();
    int index = TXT_Str2Ui(argv[2]);
    UINT sid = TXT_Str2Ui(argv[4]);

    Match_SetPattern(hMatch, index, &sid);

    return 0;
}


PLUG_API int LOGAGENT_ALERT_CmdMatchEnable(int argc, char **argv)
{
    MATCH_HANDLE hMatch = LOGAGENT_ALERT_GetCtrl();
    int index = TXT_Str2Ui(argv[2]);
    Match_Enable(hMatch, index);
    return 0;
}


PLUG_API int LOGAGENT_ALERT_CmdMatchDisable(int argc, char **argv)
{
    MATCH_HANDLE hMatch = LOGAGENT_ALERT_GetCtrl();
    int index = TXT_Str2Ui(argv[3]);
    Match_Disable(hMatch, index);
    return 0;
}

static void logagent_alert_ShowMatchedCountOne(MATCH_HANDLE hMatch, int index)
{
    if (index >= LOGAGENT_ALERT_MACTCH_COUNT) {
        return;
    }

    UINT *sid = Match_GetPattern(hMatch, index);
    UINT64 count = Match_GetMatchedCount(hMatch, index);
    int enable = Match_IsEnable(hMatch, index);

    EXEC_OutInfo("%-3d %-12u %-6u   %llu \r\n",
            index, *sid, enable, count);
}


PLUG_API int LOGAGENT_ALERT_CmdShowMatchCount(int argc, char **argv)
{
    MATCH_HANDLE hMatch = LOGAGENT_ALERT_GetCtrl();
    int i;

    EXEC_OutString("id  sid          enable   count\r\n");

    if (argc >= 5) {
        i = TXT_Str2Ui(argv[4]);
        logagent_alert_ShowMatchedCountOne(hMatch, i);
    } else {
        for (i=0; i<LOGAGENT_ALERT_MACTCH_COUNT; i++) {
            if (Match_IsEnable(hMatch, i)) {
                logagent_alert_ShowMatchedCountOne(hMatch, i);
            }
        }
    }

    return 0;
}


PLUG_API int LOGAGENT_ALERT_CmdResetMatchCount(int argc, char **argv)
{
    MATCH_HANDLE hMatch = LOGAGENT_ALERT_GetCtrl();
    int i;

    if (argc >= 5) {
        i = TXT_Str2Ui(argv[4]);
        Match_ResetMatchedCount(hMatch, i);
    } else {
        Match_ResetAllMatchedCount(hMatch);
    }

    return 0;
}

void LOGAGENT_ALERT_Save(HANDLE hFile)
{
    MATCH_HANDLE hMatch = LOGAGENT_ALERT_GetCtrl();
    int i;
    UINT *sid;

    for (i=0; i<LOGAGENT_ALERT_MACTCH_COUNT; i++) {
        sid = Match_GetPattern(hMatch, i);
        if (*sid != 0) {
            CMD_EXP_OutputCmd(hFile, "alert match %d sid %u", i, *sid);
        }
        if (Match_IsEnable(hMatch, i)) {
            CMD_EXP_OutputCmd(hFile, "alert match %d enable", i);
        }
    }
}

