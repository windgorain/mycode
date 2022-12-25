/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/cff_utl.h"
#include "utl/plug_utl.h"
#include "utl/txt_utl.h"
#include "utl/syslog_utl.h"
#include "utl/ubpf_utl.h"
#include "utl/exec_utl.h"
#include "pcap.h"
#include "../../pwatcher_pub.h"
#include "../../h/pwatcher_session.h"
#include "../../h/pwatcher_def.h"
#include "../../h/pwatcher_event.h"
#include "../../h/pwatcher_ip.h"
#include "../h/pwatcher_ob_common.h"

static int pwatcher_ob_dump_input(UINT point, PWATCHER_PKT_DESC_S *pkt, void *data);

static PWATCHER_EV_OB_S g_pwatcher_ob_node = {
    .ob_name="dump",
    .event_func=pwatcher_ob_dump_input,
};

static PWATCHER_EV_POINT_S g_pwatcher_ob_points[] = {
    {.valid=1, .ob=&g_pwatcher_ob_node, .point=PWATCHER_EV_IP_BEFORE_SESSION},
    {.valid=0, .ob=&g_pwatcher_ob_node}
};

static int g_pwatcher_ob_dump_tcp = 0;
static int g_pwatcher_ob_dump_udp = 0;
static int g_pwatcher_ob_dump_other = 0;

static int pwatcher_ob_dump_input(UINT point, PWATCHER_PKT_DESC_S *pkt, void *data)
{
    static UINT64 index = 0;

    if (pkt->tcp_header) {
        if (g_pwatcher_ob_dump_tcp) {
            printf("%llu tcp pkt: %d.%d.%d.%d:%d -> %d.%d.%d.%d:%d\r\n",
                    index,
                    IP_ADDR_4_CHAR(&pkt->ip_header->unSrcIp), ntohs(pkt->tcp_header->usSrcPort),
                    IP_ADDR_4_CHAR(&pkt->ip_header->unDstIp), ntohs(pkt->tcp_header->usDstPort));
        }
    } else if (pkt->udp_header) {
        if (g_pwatcher_ob_dump_udp) {
            printf("%llu udp pkt: %d.%d.%d.%d:%d -> %d.%d.%d.%d:%d\r\n",
                    index,
                    IP_ADDR_4_CHAR(&pkt->ip_header->unSrcIp), ntohs(pkt->udp_header->usSrcPort),
                    IP_ADDR_4_CHAR(&pkt->ip_header->unDstIp), ntohs(pkt->udp_header->usDstPort));
        }
    } else {
        if (g_pwatcher_ob_dump_other)  {
            printf("%llu ip pkt: %d.%d.%d.%d -> %d.%d.%d.%d, protocol:%u \r\n",
                    index,
                    IP_ADDR_4_CHAR(&pkt->ip_header->unSrcIp),
                    IP_ADDR_4_CHAR(&pkt->ip_header->unDstIp),
                    pkt->ip_header->ucProto);
        }
    }

    index ++;

    return 0;
}

static int pwatcher_ob_dump_init()
{
    PWatcherEvent_Reg(g_pwatcher_ob_points);

    return 0;
}

static void pwatcher_ob_dump_Finit()
{
    PWatcherEvent_UnReg(g_pwatcher_ob_points);
    Sleep(1000);
}

static void pwatcher_ob_dump_set_protocol(char *protocol, int set)
{
    if (0 == strcmp(protocol, "tcp")) {
        g_pwatcher_ob_dump_tcp = set;
    } else if (0 == strcmp(protocol, "udp")) {
        g_pwatcher_ob_dump_udp = set;
    } else if (0 == strcmp(protocol, "other")) {
        g_pwatcher_ob_dump_other = set;
    }
}

PLUG_API int PWATCHEROB_DUMP_EnableProtocol(int argc, char **argv)
{
    pwatcher_ob_dump_set_protocol(argv[3], 1);
    return 0;
}

PLUG_API int PWATCHEROB_DUMP_DisableProtocol(int argc, char **argv)
{
    pwatcher_ob_dump_set_protocol(argv[4], 0);
    return 0;
}

PLUG_API BOOL_T DllMain(PLUG_HDL hPlug, int reason, void *reserved)
{
    switch(reason) {
        case DLL_PROCESS_ATTACH:
            pwatcher_ob_dump_init();
            break;

        case DLL_PROCESS_DETACH:
            pwatcher_ob_dump_Finit();
            break;
    }

    return TRUE;
}

PLUG_ENTRY

PWATCHER_OB_FUNCTIONS 

PLUG_API int PWATCHEROB_DUMP_Save(HANDLE hFile)
{
    if (g_pwatcher_ob_dump_tcp) {
        CMD_EXP_OutputCmd(hFile, "ob dump protocol tcp");
    }
    if (g_pwatcher_ob_dump_udp) {
        CMD_EXP_OutputCmd(hFile, "ob dump protocol udp");
    }
    if (g_pwatcher_ob_dump_other) {
        CMD_EXP_OutputCmd(hFile, "ob dump protocol other");
    }

    PWATCHEROB_Save(hFile);

    return 0;
}

