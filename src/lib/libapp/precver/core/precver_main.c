/*================================================================
*   Created by LiXingang
*   Description: 报文采集器
*
================================================================*/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/ubpf_utl.h"
#include "utl/time_utl.h"
#include "comp/comp_pissuer.h"

#include "../h/precver_def.h"
#include "../h/precver_conf.h"

typedef struct {
    char *name;
    PF_PRECVER_RUN pfRun;
}PRECVER_S;

extern int PRecver_Pcap_Run(void *runner, int argc, char **argv);
extern int PRecver_Null_Run(void *runner, int argc, char **argv);
extern int PRecver_Rand_Run(void *runner, int argc, char **argv);
extern int PRecver_Ring_Run(void *runner, int argc, char **argv);
extern int PRecver_Http_Run(void *runner, int argc, char **argv);

static DLL_HEAD_S g_precver_obs = DLL_HEAD_INIT_VALUE(&g_precver_obs);

static PRECVER_S g_precvers[] = {
    {"null", PRecver_Null_Run},
    {"rand", PRecver_Rand_Run},
    {"pcap", PRecver_Pcap_Run},
    {"http", PRecver_Http_Run},
#ifdef USE_DPDK
    {"ring", PRecver_Ring_Run},
#endif
    {NULL, NULL},
};

static void precver_Run(PRECVER_RUNNER_S *runner, PRECVER_S *rcver, char *param)
{
    char *argv[32];
    int argc = 0;

    if (param != NULL) {
        argc = TXT_StrToToken(param, " \t", argv, 32);
    }

    while (1) {
        if (0 != rcver->pfRun(runner, argc, argv)) {
            IC_WrnInfo("recver run err.\r\n");
            break;
        }

        if (runner->need_stop) {
            IC_Info("recver need stop.\r\n");
            break;
        }
    }
}

int PRecver_PktInput(PRECVER_RUNNER_S *runner, PRECVER_PKT_S *pkt)
{
    runner->recv_pkt(runner, pkt);

    if (runner->need_stop) {
        return BS_STOP;
    }

    return 0;
}

int PRecver_Run(PRECVER_RUNNER_S *runner)
{
    PRECVER_S *recver = NULL;

    for (recver=g_precvers; recver->name!=NULL; recver++) {
        if (strcmp(recver->name, runner->source) == 0) {
            break;
        }
    }

    if (! recver->name) {
        RETURN(BS_NOT_FOUND);
    }

    precver_Run(runner, recver, runner->param);

    return 0;
}

char * PRecver_GetNext(char *curr/* NULL表示获取第一个 */)
{
    PRECVER_S *recver = NULL;

    if (! curr) {
        return g_precvers[0].name;
    }

    for (recver=g_precvers; recver->name!=NULL; recver++) {
        if (strcmp(recver->name, curr) == 0) {
            recver ++;
            break;
        }
    }

    return recver->name;
}

BOOL_T PRecver_IsExit(char *name)
{
    PRECVER_S *recver = NULL;

    for (recver=g_precvers; recver->name!=NULL; recver++) {
        if (strcmp(recver->name, name) == 0) {
            return TRUE;
        }
    }

    return FALSE;
}

int PRecver_Main_Init()
{
    return 0;
}
