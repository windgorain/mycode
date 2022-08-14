/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/exec_utl.h"
#include "utl/txt_utl.h"
#include "comp/comp_precver.h"
#include "comp/comp_event_hub.h"
#include "../h/precver_def.h"
#include "../h/precver_conf.h"
#include "../h/precver_worker.h"

#define PRECVER_WORKER_MAX 32

PRECVER_WORKER_S g_precver_workers[PRECVER_WORKER_MAX];

static inline void precver_worker_BindCpu(PRECVER_WORKER_S *wrk)
{
#if defined IN_LINUX && defined __X86__ && defined _GNU_SOURCE
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(wrk->cpu_index, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);
#endif
}

static void precver_worker_Main(USER_HANDLE_S *user_data)
{
    PRECVER_WORKER_S *wrk = user_data->ahUserHandle[0];
   
    if (wrk->affinity) {
        precver_worker_BindCpu(wrk);
    }

    if (BS_OK != PRecver_Run(wrk->index)) {
        return;
    }

    wrk->runner.start = 0;
    wrk->runner.need_stop = 0;
}

int PRecver_Worker_Init()
{
    int i;

    for (i=0; i<PRECVER_WORKER_MAX && g_precver_workers[i].used == 0; i++) {
        g_precver_workers[i].index = i;
        g_precver_workers[i].runner.source = g_precver_workers[i].source;
        g_precver_workers[i].runner.param = g_precver_workers[i].param;
    }

    return 0;
}

int PRecver_OneWorker_Init(int index, char *source, char *param)
{
    g_precver_workers[index].index = index;
    g_precver_workers[index].used = 1;
    
    if (source && source[0]) {
        strlcpy(g_precver_workers[index].source, source, strlen(source) + 1);
    }
    if (param && param[0]) {
        strlcpy(g_precver_workers[index].param, param, strlen(param) + 1);
    }
    g_precver_workers[index].runner.source = source;
    g_precver_workers[index].runner.param = param;

    return 0;
}

int PRecver_Worker_Sample(int index, uint8_t rate, uint8_t type)
{
    g_precver_workers[index].runner.sample_type = type;
    g_precver_workers[index].runner.sample_rate = rate;

    return 0;
}

int PRecver_Worker_NoSample(int index, uint8_t rate, uint8_t type)
{
    if (g_precver_workers[index].runner.sample_rate == rate &&
            g_precver_workers[index].runner.sample_type == type) {
        g_precver_workers[index].runner.sample_rate = 0;
        g_precver_workers[index].runner.sample_type = 0;
    }
    return 0;

}

void * PRecver_Worker_Use(int index)
{
    if (index >= PRECVER_WORKER_MAX) {
        return NULL;
    }

    g_precver_workers[index].used = 1;

    return &g_precver_workers[index];
}

void PRecver_Worker_NoUse(int index)
{
    if (index >= PRECVER_WORKER_MAX) {
        return;
    }

    g_precver_workers[index].used = 0;
    g_precver_workers[index].source[0] = 0;
    g_precver_workers[index].param[0] = 0;
    PRecver_Worker_Stop(index);
}


int PRecver_Worker_SetSource(int index, char *source)
{
    if (index >= PRECVER_WORKER_MAX) {
        RETURN(BS_OUT_OF_RANGE);
    }
    PRECVER_WORKER_S *wrk = &g_precver_workers[index];
    strlcpy(wrk->source, source, PRECVER_WORKER_SOURCE_SIZE);
    return 0;
}

int PRecver_Worker_SetParam(int index, char *param)
{
    if (index >= PRECVER_WORKER_MAX) {
        RETURN(BS_OUT_OF_RANGE);
    }
    PRECVER_WORKER_S *wrk = &g_precver_workers[index];
    wrk->param[0] = 0;
    if (param) {
        strlcpy(wrk->param, param, PRECVER_WORKER_PARAM_SIZE);
    }
    return 0;
}

int PRecver_Worker_SetAffinity(int index, int cpu_index)
{
    if (index >= PRECVER_WORKER_MAX) {
        RETURN(BS_OUT_OF_RANGE);
    }
    PRECVER_WORKER_S *wrk = &g_precver_workers[index];
    wrk->cpu_index = cpu_index;
    wrk->affinity = 1;
    return 0;
}

int PRecver_Worker_ClrAffinity(int index)
{
    if (index >= PRECVER_WORKER_MAX) {
        RETURN(BS_OUT_OF_RANGE);
    }
    PRECVER_WORKER_S *wrk = &g_precver_workers[index];
    wrk->affinity = 0;
    wrk->cpu_index = 0;
    return 0;
}

int PRecver_Worker_Start(int index)
{
    if (index >= PRECVER_WORKER_MAX) {
        RETURN(BS_OUT_OF_RANGE);
    }
    PRECVER_WORKER_S *wrk = &g_precver_workers[index];
    USER_HANDLE_S ud;

    if (wrk->runner.start) {
        EXEC_OutString("Worker is running.\r\n");
        return BS_OK;
    }

    if (wrk->source[0] == '\0') {
        EXEC_OutString("Souce has not configed.\r\n");
        RETURN(BS_NOT_READY);
    }

    ud.ahUserHandle[0] = wrk;

    wrk->runner.start = 1;
    wrk->runner.need_stop = 0;

    char name[32];
    sprintf(name, "precver%d", index);
    THREAD_Create(name, NULL, precver_worker_Main, &ud);

    return 0;
}

int PRecver_Worker_Stop(int index)
{
    if (index >= PRECVER_WORKER_MAX) {
        RETURN(BS_OUT_OF_RANGE);
    }
    PRECVER_WORKER_S *wrk = &g_precver_workers[index];

    if (! wrk->runner.start) {
        EXEC_OutString("Worker has stopped.\r\n");
        return BS_OK;
    }
    wrk->runner.need_stop = 1;
    return 0;
}

void PRecver_Worker_Walk(PF_PRECVER_WORK_WALK walk, void *ud)
{
    int i;

    for (i=0; i<PRECVER_WORKER_MAX; i++) {
        walk(&g_precver_workers[i], ud);
    }
}

PRECVER_RUNNER_S* PRecver_Worker_GetRunner(int index)
{
    if (index < 0 || index >= PRECVER_WORKER_MAX) {
        return NULL;
    }

    return &(g_precver_workers[index].runner);
}

