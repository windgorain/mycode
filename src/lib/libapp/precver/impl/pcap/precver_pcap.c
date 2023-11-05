/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/getopt2_utl.h"
#include "comp/comp_precver.h"
#include "pcap.h"
#include "../../h/precver_def.h"
#include "../../h/precver_ev.h"
#include "../precver_impl_common.h"

typedef struct {
    pcap_t *pcap_handle;
    char *dev;
    UINT is_read: 1;
    UINT is_read_loop: 1;
}PRECVER_PCAP_S;

static int precver_pcap_help(GETOPT2_NODE_S *opts)
{
    char buf[512];
    printf("%s\r\n", GETOPT2_BuildHelpinfo(opts, buf, sizeof(buf)));
    return 0;
}

static inline void precver_pcap_close(PRECVER_RUNNER_S *runner)
{
    PRECVER_PCAP_S *handle = runner->recver_handle;

    if (handle->dev) {
        free(handle->dev);
    }

    if (handle->pcap_handle) {
        pcap_close(handle->pcap_handle);
    }

    MEM_Free(handle);

    runner->recver_handle = NULL;
    runner->need_stop = 1;
}

static int precver_pcap_reopen(PRECVER_PCAP_S *handle)
{
    char errbuf[PCAP_ERRBUF_SIZE];

    if (handle->pcap_handle) {
        pcap_close(handle->pcap_handle);
    }

    handle->pcap_handle = pcap_open_offline(handle->dev, errbuf);
    if (! handle->pcap_handle) {
        RETURN(BS_ERR);
    }

    return 0;
}

static void * precver_pcap_open(int argc, char **argv)
{
    char errbuf[PCAP_ERRBUF_SIZE];
    PRECVER_PCAP_S *handle;
    char *source = NULL;
    GETOPT2_NODE_S opts[] = {
        {'o', 'h', "help", GETOPT2_V_NONE, NULL, NULL, 0},
        {'o', 'r', "read", GETOPT2_V_STRING, &source, "read pcap file", 0},
        {'o', 'l', "loop", GETOPT2_V_NONE, NULL, "read loop", 0},
        {'o', 'i', "interface", GETOPT2_V_STRING, &source, "interface name", 0},
        {0}
    };

    if (argc < 2) {
        precver_pcap_help(opts);
        return NULL;
    }

	if (BS_OK != GETOPT2_ParseFromArgv0(argc, argv, opts)) {
        precver_pcap_help(opts);
        return NULL;
    }

    if (!source) {
        return NULL;
    }

    handle = MEM_ZMalloc(sizeof(PRECVER_RUNNER_S));
    if (!handle) {
        return NULL;
    }

    handle->dev = strdup(source);
    if (! handle->dev) {
        MEM_Free(handle);
        return NULL;
    }

    if (GETOPT2_IsOptSetted(opts, 'r', NULL)) {
        handle->is_read = 1;
		handle->pcap_handle = pcap_open_offline(source, errbuf);
	}else if (GETOPT2_IsOptSetted(opts, 'i', NULL)) {
		handle->pcap_handle = pcap_open_live(source, 65535, 1, 1000, errbuf);
	}

    if (GETOPT2_IsOptSetted(opts, 'l', NULL)) {
        handle->is_read_loop = 1;
    }

    if (! handle->pcap_handle) {
        free(handle->dev);
        MEM_Free(handle);
        return NULL;
    }

    return handle;
}

static void precver_pcap_input(unsigned char* argument,
        const struct pcap_pkthdr* packet_header,
        const unsigned char* packet_content)
{
    PRECVER_PKT_S pkt;
    PRECVER_RUNNER_S *runner = (void*)argument;

    pkt.data = (void*)packet_content;
    pkt.data_len = packet_header->caplen;
    pkt.ts = packet_header->ts;

    PRecver_Ev_Publish(runner, &pkt);
    if (runner->need_stop) {
        pcap_breakloop(runner->recver_handle);
    }

    return;
}

static inline void precver_pcap_process_read(PRECVER_RUNNER_S *runner)
{
    PRECVER_PCAP_S *handle = runner->recver_handle;

    if (! handle->is_read) {
        return;
    }

    if (handle->is_read_loop) {
        if (0 != precver_pcap_reopen(handle)) {
            precver_pcap_close(runner);
        }
    } else {
        precver_pcap_close(runner);
    }
}

PLUG_API int PRecverImpl_Init(PRECVER_RUNNER_S *runner, int argc, char **argv)
{
    void * handle = precver_pcap_open(argc, argv);
    if (NULL == handle) {
        RETURN(BS_CAN_NOT_OPEN);
    }
    runner->recver_handle = handle;
    return 0;
}

PLUG_API int PRecverImpl_Run(PRECVER_RUNNER_S *runner)
{
    PRECVER_PCAP_S *handle;

    if (! runner->recver_handle) {
        runner->need_stop = 1;
        return 0;
    }

    handle = runner->recver_handle;

    if (0 == pcap_dispatch(handle->pcap_handle, -1, precver_pcap_input, (void*)runner)) {
        precver_pcap_process_read(runner);
    }

    return 0;
}

