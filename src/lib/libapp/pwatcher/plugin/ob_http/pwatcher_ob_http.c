/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/cff_utl.h"
#include "utl/bit_opt.h"
#include "utl/http_lib.h"
#include "utl/nap_utl.h"
#include "utl/http_monitor.h"
#include "utl/plug_utl.h"
#include "../../h/pwatcher_session.h"
#include "../../h/pwatcher_def.h"
#include "../../h/pwatcher_event.h"
#include "../../h/pwatcher_worker.h"
#include "../h/pwatcher_ob_common.h"

#define PWATCHER_OB_HTTP_DBG_PACKET 0x1
#define PWATCHER_OB_HTTP_DBG_ERROR  0x2

#define PWATCHER_OB_HTTP_MAX_PORT 32

static int pwatcher_ob_http_input(UINT point, PWATCHER_PKT_DESC_S *pkt, void *data);

static PWATCHER_EV_OB_S g_pwatcher_ob_node = {
    .ob_name="http",
    .event_func=pwatcher_ob_http_input
};
static PWATCHER_EV_POINT_S g_pwatcher_ob_points[] = {
    {.valid=1, .ob=&g_pwatcher_ob_node, .point=PWATCHER_EV_TCP_FIRST_PAYLOAD},
    {.valid=0, .ob=&g_pwatcher_ob_node}
};
static USHORT g_pwatcher_ob_http_ports[PWATCHER_OB_HTTP_MAX_PORT] = {80, 0};

static BOOL_T pwatcher_ob_http_is_http_port(USHORT port)
{
    int i;
    for (i=0; i<PWATCHER_OB_HTTP_MAX_PORT; i++) {
        if (g_pwatcher_ob_http_ports[i] == 0) {
            return FALSE;
        }
        if (g_pwatcher_ob_http_ports[i] == port) {
            return TRUE;
        }
    }

    return FALSE;
}

static void pwatcher_ob_http_tcp_first_payload_input(PWATCHER_PKT_DESC_S *pkt)
{
    PWATCHER_HTTP_S http;
    HTTP_HEAD_PARSER hHeadParser;
    int ret;

    if ((!pwatcher_ob_http_is_http_port(pkt->tcp_header->usDstPort)) 
            &&(!pwatcher_ob_http_is_http_port(pkt->tcp_header->usSrcPort))) {
        return;
    }

    if (! HTTP_IsHttpHead(pkt->l4_payload, pkt->l4_payload_len)) {
        return;
    }
    
    hHeadParser = HTTP_CreateHeadParser();
    if (! hHeadParser) {
        return;
    }

    UINT head_len = HTTP_GetHeadLen(pkt->l4_payload, pkt->l4_payload_len);
    if (head_len == 0) {
        head_len = pkt->l4_payload_len;
    }

    if (! pkt->direct_s2c) {
        ret = HTTP_ParseHead(hHeadParser, pkt->l4_payload, head_len, HTTP_REQUEST);
    } else {
        ret = HTTP_ParseHead(hHeadParser, pkt->l4_payload, head_len, HTTP_RESPONSE);
    }

    if (ret != 0) {
        HTTP_DestoryHeadParser(hHeadParser);
        return;
    }

    memset(&http, 0, sizeof(http));

    if (!pkt->direct_s2c) {
        CHAR *ua = HTTP_GetHeadField(hHeadParser, HTTP_FIELD_USER_AGENT);
        if (ua) {
            PWatcherEvent_Notify(PWATCHER_EV_HTTP_UA, pkt, ua);
        }

        CHAR *url = HTTP_GetUriPath(hHeadParser);
        if (url) {
            PWatcherEvent_Notify(PWATCHER_EV_HTTP_URL, pkt, ua);
        }   
    }

    http.head_parser = hHeadParser;
    http.datatype = HTTP_MONITOR_DATA_HEAD;
    http.data = pkt->l4_payload;
    http.datalen = head_len;

    PWatcherEvent_Notify(PWATCHER_EV_HTTP, pkt, &http);

    HTTP_DestoryHeadParser(hHeadParser);

    return;
}

static int pwatcher_ob_http_input(UINT point, PWATCHER_PKT_DESC_S *pkt, void *ob_data)
{
    switch (point) {
        case PWATCHER_EV_TCP_FIRST_PAYLOAD:
            pwatcher_ob_http_tcp_first_payload_input(pkt);
            break;
    }

    return 0;
}

static int pwatcher_ob_http_init()
{
    PWatcherEvent_Reg(g_pwatcher_ob_points);

    int i;
    for (i=0; i<PWATCHER_OB_HTTP_MAX_PORT; i++) {
        g_pwatcher_ob_http_ports[i] = htons(g_pwatcher_ob_http_ports[i]);
    }

    return 0;
}

static void pwatcher_ob_http_Finit()
{
    PWatcherEvent_UnReg(g_pwatcher_ob_points);
    Sleep(1000);
}

PLUG_API BOOL_T DllMain(PLUG_HDL hPlug, int reason, void *reserved)
{
    switch(reason) {
        case DLL_PROCESS_ATTACH:
            pwatcher_ob_http_init();
            break;

        case DLL_PROCESS_DETACH:
            pwatcher_ob_http_Finit();
            break;
    }

    return TRUE;
}

PLUG_ENTRY

PWATCHER_OB_FUNCTIONS 

