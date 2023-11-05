/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _PWATCHER_RUNNER_H
#define _PWATCHER_RUNNER_H
#ifdef __cplusplus
extern "C"
{
#endif

int PWatcherRunner_Init();
unsigned int PWatcherRunner_PktInput(void *data, USHORT data_len, struct timeval *ts);
void PWatcherRunner_Timer();

#ifdef __cplusplus
}
#endif
#endif 
