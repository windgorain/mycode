/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _LOGAGENT_HTTP_H
#define _LOGAGENT_HTTP_H
#ifdef __cplusplus
extern "C"
{
#endif

#define LOGAGENT_HTTP_MACTCH_COUNT 128

HTTP_LOG_S * LOGAGENT_HTTP_GetCtrl();
HTTP_LOG_S * LOGAGENT_HTTP_GetMatch();
void LOGAGENT_HTTP_Save(HANDLE hFile);

#ifdef __cplusplus
}
#endif
#endif //LOGAGENT_HTTP_H_
