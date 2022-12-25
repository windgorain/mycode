/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/cff_utl.h"
#include "utl/txt_utl.h"
#include "utl/exec_utl.h"
#include "utl/plug_utl.h"
#include "utl/pcap_file.h"
#include "utl/getopt2_utl.h"
#include "../../h/pwatcher_session.h"
#include "../../h/pwatcher_def.h"
#include "../../h/pwatcher_event.h"
#include "../h/pwatcher_ob_common.h"

typedef struct {
    UBPF_JIT_S bpf;
}PWATCHER_PCPA_BPF_S;

typedef struct {
    FILE *pcap_file;
    UINT time_limit;
    UINT time;
    UINT64 count_limit;
    UINT64 count;
    PWATCHER_PCPA_BPF_S *bpf;
    MUTEX_S lock;
}PWATCHER_PCAP_SVR_S;

static int pwatcher_ob_pcap_input(UINT point, PWATCHER_PKT_DESC_S *pkt, void *data);
static PWATCHER_EV_OB_S g_pwatcher_ob_node = {
    .ob_name="pcap",
    .event_func=pwatcher_ob_pcap_input
};
static PWATCHER_EV_POINT_S g_pwatcher_ob_points[] = {
    {.valid=1, .ob=&g_pwatcher_ob_node, .point=PWATCHER_EV_IP_BEFORE_SESSION},
    {.valid=1, .ob=&g_pwatcher_ob_node, .point=PWATCHER_EV_GLOBAL_TIMER},
    {.valid=0, .ob=&g_pwatcher_ob_node}
};

static PWATCHER_PCAP_SVR_S g_pwatcher_pcap_svr;

static void pwatcher_ob_pcap_close(PWATCHER_PCAP_SVR_S *svr)
{
    if (svr->pcap_file) {
        PCAPFILE_Close(svr->pcap_file);
        svr->pcap_file = NULL;
    }
    if (svr->bpf) {
        RcuEngine_Free(svr->bpf);
        svr->bpf = NULL;
    }
}

static void pwatcher_ob_pcap_record(PWATCHER_PCAP_SVR_S *svr, PWATCHER_PKT_DESC_S *pkt)
{
    FILE *fp = svr->pcap_file;

    if (! fp) {
        return;
    }

    PCAPFILE_WritePkt(fp, pkt->recver_pkt.data, &pkt->recver_pkt.ts,
            pkt->recver_pkt.data_len, pkt->recver_pkt.data_len);

    svr->count ++;

    if (svr->count_limit) {
        if (svr->count >= svr->count_limit) {
            pwatcher_ob_pcap_close(svr);
        }
    }

    return;
}

static inline int pwatcher_ob_pcap_bpf_exec(PWATCHER_PCPA_BPF_S *bpf, PWATCHER_PKT_DESC_S* pkt)
{
    int ret = 1;

    ubpf_jit_fn jit_func = bpf->bpf.jit_func;
    if(jit_func) {
        ret = jit_func(pkt->ip_header, pkt->ip_pkt_len);
    }

    return ret;
}

static inline void pwatcher_ob_pcap_process_pkt(PWATCHER_PKT_DESC_S *pkt)
{
    PWATCHER_PCAP_SVR_S *svr = &g_pwatcher_pcap_svr;
    PWATCHER_PCPA_BPF_S *bpf = svr->bpf;

    if (! svr->pcap_file) {
        return;
    }

    if (bpf && (0==pwatcher_ob_pcap_bpf_exec(bpf, pkt))) {
        return;
    }

    MUTEX_P(&svr->lock);
    if (svr->pcap_file) {
        pwatcher_ob_pcap_record(svr, pkt);
    }
    MUTEX_V(&svr->lock);
}

static inline void pwatcher_ob_pcap_timer_locked(PWATCHER_PCAP_SVR_S *svr)
{
    if (! svr->pcap_file) {
        return;
    }

    if (! svr->time_limit) {
        return;
    }

    svr->time ++;

    if (svr->time < svr->time_limit) {
        return;
    }

    pwatcher_ob_pcap_close(svr);
}

static inline void pwatcher_ob_pcap_timer()
{
    PWATCHER_PCAP_SVR_S *svr = &g_pwatcher_pcap_svr;

    if (! svr->pcap_file) {
        return;
    }

    if (! svr->time_limit) {
        return;
    }

    MUTEX_P(&svr->lock);
    pwatcher_ob_pcap_timer_locked(svr);
    MUTEX_V(&svr->lock);

}

static int pwatcher_ob_pcap_input(UINT point, PWATCHER_PKT_DESC_S *pkt, void *data)
{
    switch (point) {
        case PWATCHER_EV_IP_BEFORE_SESSION:
            pwatcher_ob_pcap_process_pkt(pkt);
            break;
        case PWATCHER_EV_GLOBAL_TIMER:
            pwatcher_ob_pcap_timer();
            break;
    }

    return 0;
}

static int pwatcher_ob_pcap_init()
{
    MUTEX_Init(&g_pwatcher_pcap_svr.lock);
    PWatcherEvent_Reg(g_pwatcher_ob_points);
    return 0;
}

static void pwatcher_ob_pcap_finit()
{
    PWatcherEvent_UnReg(g_pwatcher_ob_points);
    Sleep(1000);
    MUTEX_Final(&g_pwatcher_pcap_svr.lock);
}

PLUG_API BOOL_T DllMain(PLUG_HDL hPlug, int reason, void *reserved)
{
    switch(reason) {
        case DLL_PROCESS_ATTACH:
            pwatcher_ob_pcap_init();
            break;

        case DLL_PROCESS_DETACH:
            pwatcher_ob_pcap_finit();
            break;
    }

    return TRUE;
}

PLUG_ENTRY

PWATCHER_OB_FUNCTIONS 

static int pwatcher_pcap_set_filter(PWATCHER_PCAP_SVR_S *svr, char *cbpf_string)
{
    PWATCHER_PCPA_BPF_S *bpf = NULL;

    if(cbpf_string != NULL) {
        bpf = RcuEngine_Malloc(sizeof(PWATCHER_PCPA_BPF_S));
        if (NULL == bpf) {
            RETURN(BS_NO_MEMORY);
        }
        if (BS_OK != UBPF_S2j(DLT_RAW, cbpf_string, &bpf->bpf)) {
            EXEC_OutString("Can't compile bpf string \r\n");
            RcuEngine_Free(bpf);
            RETURN(BS_ERR);
        }
    }

    svr->bpf = bpf;

    return 0;
}

static int pwatcher_pcap_open_file(PWATCHER_PCAP_SVR_S *svr, char *file)
{
    char filename[256];
    scnprintf(filename, sizeof(filename), "pcap/%s", file);

    FILE * pcap_file = PCAPFILE_Open(filename, "wb+");

    if (pcap_file) {
        PCAPFILE_WriteHeader(pcap_file, DLT_EN10MB);
    } else {
        EXEC_OutString("Can't open file \r\n");
        RETURN(BS_CAN_NOT_OPEN);
    }

    svr->pcap_file = pcap_file;

    return 0;
}

static int pwatcher_ob_pcap_cmd_help(GETOPT2_NODE_S *opts)
{
    char buf[512];
    EXEC_OutInfo("%s\r\n", GETOPT2_BuildHelpinfo(opts, buf, sizeof(buf)));
    return 0;
}

/* ob pcap run -w filename -c count -t seconds -f filter */
PLUG_API int PWatcherObPcap_CmdRun(int argc, char **argv)
{
    PWATCHER_PCAP_SVR_S *svr = &g_pwatcher_pcap_svr;
    void *filter = NULL;
    char *file;
    GETOPT2_NODE_S opts[] = {
        {'o', 'w', "write", 's', &file, "write to file", 0},
        {'o', 'c', "count", 'u', &svr->count_limit, "read pcap file", 0},
        {'o', 't', "time", 'u', &svr->time_limit, "seconds", 0},
        {'o', 'f', "filter", 's', &filter, "seconds", 0},
        {0}
    };

    if (svr->pcap_file) {
        EXEC_OutString("Already running \r\n");
        RETURN(BS_ALREADY_EXIST);
    }

    svr->time_limit = 0;
    svr->time = 0;
    svr->count_limit = 0;
    svr->count = 0;

	if (BS_OK != GETOPT2_ParseFromArgv0(argc-3, argv+3, opts)) {
        pwatcher_ob_pcap_cmd_help(opts);
        return -1;
    }

    if (file == NULL) {
        file = "dump.pcap";
    }

    if (0 != pwatcher_pcap_set_filter(svr, filter)) {
        pwatcher_ob_pcap_close(svr);
        RETURN(BS_ERR);
    }

    if (0 != pwatcher_pcap_open_file(svr, file)) {
        pwatcher_ob_pcap_close(svr);
        RETURN(BS_ERR);
    }

    return 0;
}

PLUG_API int PWatcherObPcap_CmdStop(int argc, char **argv)
{
    PWATCHER_PCAP_SVR_S *svr = &g_pwatcher_pcap_svr;

    if (! svr->pcap_file) {
        return 0;
    }

    MUTEX_P(&svr->lock);
    pwatcher_ob_pcap_close(svr);
    MUTEX_V(&svr->lock);

    return 0;
}

