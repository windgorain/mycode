/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _LOG_AGENT_TCP_H
#define _LOG_AGENT_TCP_H
#ifdef __cplusplus
extern "C"
{
#endif

#define LOGAGENT_TCP_MACTCH_COUNT 128

TCP_LOG_S * LOGAGENT_TCP_GetCtrl();
MATCH_HANDLE LOGAGENT_TCP_GetMatch();

void LOGAGENT_TCP_Save(HANDLE hFile);

#ifdef __cplusplus
}
#endif
#endif 
