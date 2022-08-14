/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _PRECVER_EV_H
#define _PRECVER_EV_H
#ifdef __cplusplus
extern "C"
{
#endif

void PRecver_Ev_Publish(void *runner, void *pkt);
void PRecver_Ev_TimeStep(void *runner);

#ifdef __cplusplus
}
#endif
#endif //PRECVER_EV_H_
