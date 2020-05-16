/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/txt_utl.h"
#include "comp/comp_precver.h"
#include "../h/precver_def.h"
#include "../h/precver_worker.h"

static int precver_cmd_GetWorker(void *env)
{
    UINT index;
    CHAR *pcModeValue = CMD_EXP_GetCurrentModeValue(env);
    TXT_Atoui(pcModeValue, &index);

    return index;
}

/* worker %INT<0-31> */
PLUG_API int PRecver_Cmd_EnterWorker(int argc, char **argv, void *env)
{
    UINT index;

    TXT_Atoui(argv[1], &index);
    if (NULL == PRecver_Worker_Use(index)) {
        RETURN(BS_OUT_OF_RANGE);
    }

    return BS_OK;
}

/* no worker %INT<0-31> */
PLUG_API int PRecver_Cmd_NoWorker(int argc, char **argv, void *env)
{
    UINT index;
    TXT_Atoui(argv[2], &index);
    PRecver_Worker_NoUse(index);
    return BS_OK;
}

/* source %STRING */
PLUG_API int PRecver_Cmd_SetWorkerSource(int argc, char **argv, void *env)
{
    int index = precver_cmd_GetWorker(env);
    return PRecver_Worker_SetSource(index, argv[1]);
}

/* param %STRING */
PLUG_API int PRecver_Cmd_SetWorkerParam(int argc, char **argv, void *env)
{
    int index = precver_cmd_GetWorker(env);
    return PRecver_Worker_SetParam(index, argv[1]);
}

/* affinity %INT */
PLUG_API int PRecver_Cmd_SetWorkerAffinity(int argc, char **argv, void *env)
{
    int index = precver_cmd_GetWorker(env);
    UINT cpu_index;
    TXT_Atoui(argv[1], &cpu_index);
    return PRecver_Worker_SetAffinity(index, cpu_index);
}

/* no affinity */
PLUG_API int PRecver_Cmd_ClrWorkerAffinity(int argc, char **argv, void *env)
{
    int index = precver_cmd_GetWorker(env);
    return PRecver_Worker_ClrAffinity(index);
}

/* start */
PLUG_API int PRecver_Cmd_WorkerStart(int argc, char **argv, void *env)
{
    int index = precver_cmd_GetWorker(env);
    return PRecver_Worker_Start(index);
}

/* no start */
PLUG_API int PRecver_Cmd_WorkerStop(int argc, char **argv, void *env)
{
    int index = precver_cmd_GetWorker(env);
    return PRecver_Worker_Stop(index);
}

static void precver_cmd_SaveWorker(HANDLE file, PRECVER_WORKER_S *wrk)
{
    if (wrk->affinity) {
        CMD_EXP_OutputCmd(file, "affinity %u", wrk->cpu_index);
    }

    if (wrk->source[0] != 0) {
        CMD_EXP_OutputCmd(file, "source %s", wrk->source);
    }

    if (wrk->param[0] != 0) {
        CMD_EXP_OutputCmd(file, "param %s", wrk->param);
    }

    if ((wrk->runner.start) && (! wrk->runner.need_stop)) {
        CMD_EXP_OutputCmd(file, "start");
    }
}

static void precer_cmd_save(void *worker, void *ud)
{
    PRECVER_WORKER_S *wrk = worker;

    if (0 == wrk->used) {
        return;
    }

    CMD_EXP_OutputMode(ud, "worker %d", wrk->index);
    precver_cmd_SaveWorker(ud, wrk);
    CMD_EXP_OutputModeQuit(ud);
}

PLUG_API int PRecver_Cmd_Save(HANDLE hFile)
{
    PRecver_Worker_Walk(precer_cmd_save, hFile);
    return BS_OK;
}

