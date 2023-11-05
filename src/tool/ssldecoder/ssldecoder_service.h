/*================================================================
*   Created by LiXingang: 2018.11.14
*   Description: 
*
================================================================*/
#ifndef _SSLDECODER_SERVICE_H
#define _SSLDECODER_SERVICE_H
#ifdef __cplusplus
extern "C"
{
#endif

int ssldecoder_service_init();
int ssldecoder_service_open_tcp(unsigned short port);
void ssldecoder_service_run();

#ifdef __cplusplus
}
#endif
#endif 
