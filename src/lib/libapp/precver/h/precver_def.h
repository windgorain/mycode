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

typedef struct {
    UINT used:1;
    UINT affinity:1;
    UINT index:8;
    UINT cpu_index:8;
    char source[PRECVER_WORKER_SOURCE_SIZE];
    char param[PRECVER_WORKER_PARAM_SIZE];

    PRECVER_RUNNER_S runner;
}PRECVER_WORKER_S;

typedef int (*PF_PRECVER_RUN)(void *worker, int argc, char **argv);

int PRecver_Init();
int PRecver_PktInput(PRECVER_RUNNER_S *runner, PRECVER_PKT_S *pkt);

int PRecver_Main_Init();
int PRecver_Comp_Init();

#ifdef __cplusplus
}
#endif
#endif //PRECVER_DEF_H_
