/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/getopt2_utl.h"
#include "comp/comp_precver.h"
#include "pcap.h"
#include "../h/precver_def.h"

static int precver_pcap_help(GETOPT2_NODE_S *opts)
{
    char buf[512];
    printf("%s\r\n", GETOPT2_BuildHelpinfo(opts, buf, sizeof(buf)));
    return 0;
}

static pcap_t * precver_pcap_open(int argc, char **argv)
{
    char errbuf[PCAP_ERRBUF_SIZE];
    static char *source = NULL;
    static GETOPT2_NODE_S opts[] = {
        {'o', 'h', "help", 0, NULL, NULL, 0},
        {'o', 'r', "read", 's', &source, "read pcap file", 0},
        {'o', 'i', "interface", 's', &source, "interface name", 0},
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
    
    if (GETOPT2_IsOptSetted(opts, 'r', NULL)) {
		return pcap_open_offline(source, errbuf);
	}else if (GETOPT2_IsOptSetted(opts, 'i', NULL)) {
		return pcap_open_live(source, 65535, 1, 1000, errbuf);
	}

    return NULL;
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

    if (BS_STOP == PRecver_PktInput(runner, &pkt)) {
        pcap_breakloop(runner->recver_handle);
    }
}

int PRecver_Pcap_Run(PRECVER_RUNNER_S *runner, int argc, char **argv)
{
    pcap_t * handle = precver_pcap_open(argc, argv);
    if (NULL == handle) {
        RETURN(BS_CAN_NOT_OPEN);
    }

    runner->recver_handle = handle;
    pcap_dispatch(handle, -1, precver_pcap_input, (void*)runner);
    pcap_close(handle);
    runner->recver_handle = NULL;

    return 0;
}

