/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _PRECVER_DEF_H
#define _PRECVER_DEF_H

#ifdef __cplusplus
extern "C"
{
#endif

#define PRECVER_WORKER_PARAM_SIZE 128
#define PRECVER_WORKER_SOURCE_SIZE 32

typedef int (*PF_PRecverImpl_Init)(void *runner, int argc, char **argv);
typedef int (*PF_PRecverImpl_Run)(void *runner);

typedef struct {
    UINT used:1;
    UINT affinity:1;
	UINT sample_type:2;
    UINT index:8;
    UINT cpu_index:8;
    UINT sample_rate:8;
    char source[PRECVER_WORKER_SOURCE_SIZE];
    char param[PRECVER_WORKER_PARAM_SIZE];

    PRECVER_RUNNER_S runner;
}PRECVER_WORKER_S;

int PRecver_Init();

int PRecver_Main_Init();
int PRecver_Comp_Init();

#ifdef __cplusplus
}
#endif
#endif //PRECVER_DEF_H_
