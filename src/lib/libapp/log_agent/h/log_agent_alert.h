/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _LOG_AGENT_ALERT_H
#define _LOG_AGENT_ALERT_H
#ifdef __cplusplus
extern "C"
{
#endif

#define LOGAGENT_ALERT_MACTCH_COUNT 128

MATCH_HANDLE LOGAGENT_ALERT_GetCtrl();
void LOGAGENT_ALERT_Save(HANDLE hFile);

#ifdef __cplusplus
}
#endif
#endif //LOG_AGENT_ALERT_H_
