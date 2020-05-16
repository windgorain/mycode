/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _PRECVER_CONF_H
#define _PRECVER_CONF_H
#ifdef __cplusplus
extern "C"
{
#endif

int PRecver_Conf_Init();
int PRecver_Conf_GetWorkerNum();
void * PRecver_Conf_Open(char *ini_path);

#ifdef __cplusplus
}
#endif
#endif //PRECVER_CONF_H_
