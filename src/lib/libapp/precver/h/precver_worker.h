/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _PRECVER_WORKER_H
#define _PRECVER_WORKER_H
#ifdef __cplusplus
extern "C"
{
#endif

int PRecver_Worker_Init();
void * PRecver_Worker_Use(int index);
void PRecver_Worker_NoUse(int index);
int PRecver_Worker_SetSource(int index, char *source);
int PRecver_Worker_SetParam(int index, char *param);
int PRecver_Worker_SetAffinity(int index, int cpu_index);
int PRecver_Worker_ClrAffinity(int index);
int PRecver_Worker_Start(int index);
int PRecver_Worker_Stop(int index);

typedef void (*PF_PRECVER_WORK_WALK)(void *worker, void *ud);
void PRecver_Worker_Walk(PF_PRECVER_WORK_WALK walk, void *ud);

#ifdef __cplusplus
}
#endif
#endif //PRECVER_WORKER_H_
