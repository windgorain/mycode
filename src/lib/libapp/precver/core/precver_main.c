/*================================================================
*   Created by LiXingang
*   Description: 报文采集器
*
================================================================*/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/args_utl.h"
#include "utl/ubpf_utl.h"
#include "utl/time_utl.h"
#include "comp/comp_event_hub.h"

#include "../h/precver_def.h"
#include "../h/precver_conf.h"
#include "../h/precver_plug.h"
#include "../h/precver_ev.h"
#include "../h/precver_worker.h"

int PRecver_Run(int index)
{
    char *argv[32];
    int argc = 0;
    PLUG_HDL hplug;
    PF_PRecverImpl_Init pfinit;
    PF_PRecverImpl_Run pfrun;

    PRECVER_RUNNER_S *runner = PRecver_Worker_GetRunner(index);
    if (!runner) {
        fprintf(stderr, "Can't load %d \r\n", index);
        RETURN(BS_ERR);
    }

    if (0 != PRecverPlug_LoadPlug(runner->source)) {
        fprintf(stderr, "Can't load %s \r\n", runner->source);
        RETURN(BS_ERR);
    }

    hplug = PRecverPlug_GetPlug(runner->source);
    if (! hplug) {
        fprintf(stderr, "Can't load %s \r\n", runner->source);
        RETURN(BS_ERR);
    }

    pfinit = PLUG_GET_FUNC_BY_NAME(hplug, "PRecverImpl_Init");
    pfrun = PLUG_GET_FUNC_BY_NAME(hplug, "PRecverImpl_Run");
    if ((!pfinit) || (!pfrun)) {
        fprintf(stderr, "Can't load %s \r\n", runner->source);
        RETURN(BS_ERR);
    }

    if (runner->param != NULL) {
        argc = ARGS_Split(runner->param, argv, 32);
    }

    int ret = pfinit(runner, argc, argv);
    if (0 != ret) {
        fprintf(stderr, "Can't init recver \r\n");
        return ret;
    }

    /* 循环调用runner, 至少每隔一秒调用一次timer_step, 所以在ruuner收不到报文情况下,需要1秒或1秒内返回 */
    while (1) {
        if (0 != pfrun(runner)) {
            IC_WrnInfo("recver run err.\r\n");
            break;
        }
        if (runner->need_stop) {
            IC_Info("recver stop.\r\n");
            break;
        }
        PRecver_Ev_TimeStep(runner);
    }

    return 0;
}

char * PRecver_GetNext(char *curr/* NULL表示获取第一个 */)
{
    return PRecverPlug_CfgGetNext(curr);
}

BOOL_T PRecver_IsExit(char *name)
{
    return PRecverPlug_CfgIsExist(name);
}

int PRecver_Main_Init()
{
    return 0;
}
