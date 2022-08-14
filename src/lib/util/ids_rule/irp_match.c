/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/txt_utl.h"
#include "utl/ip_utl.h"
#include "utl/ip_string.h"
#include "utl/ip_protocol.h"
#include "utl/socket_utl.h"
#include "utl/kv_utl.h"
#include "utl/lstr_utl.h"
#include "utl/ldata_utl.h"
#include "utl/irp_utl.h"
#include "pcre.h"
#include "irp_def.h"

typedef struct {
    IRP_PKT_INFO_S *pkt_info;
    PF_IRP_MATCH_FUNC func;
    void *ud;
}IRP_MATCH_UD_S;

static inline BOOL_T irp_match_rtn_ip_out(IRP_RTN_S *rtn, IRP_IP_S *addr)
{
    if (! rtn->dir_out) {
        return FALSE;
    }

    if (rtn->sip.uiIP != (addr->sip & rtn->sip.uiMask)) {
        return FALSE;
    }

    if (rtn->dip.uiIP != (addr->dip & rtn->dip.uiMask)) {
        return FALSE;
    }

    if (rtn->sport) {
        if (rtn->sport != addr->sport) {
            return FALSE;
        }
    }

    if (rtn->dport) {
        if (rtn->dport != addr->dport) {
            return FALSE;
        }
    }

    return TRUE;
}

static inline BOOL_T irp_match_rtn_ip_in(IRP_RTN_S *rtn, IRP_IP_S *addr)
{
    if (! rtn->dir_in) {
        return FALSE;
    }

    if (rtn->sip.uiIP != (addr->dip & rtn->sip.uiMask)) {
        return FALSE;
    }

    if (rtn->dip.uiIP != (addr->sip & rtn->dip.uiMask)) {
        return FALSE;
    }

    if (rtn->sport) {
        if (rtn->sport != addr->dport) {
            return FALSE;
        }
    }

    if (rtn->dport) {
        if (rtn->dport != addr->sport) {
            return FALSE;
        }
    }

    return TRUE;
}

static BOOL_T irp_match_rtn(IRP_RTN_S *rtn, IRP_IP_S *addr)
{
    if (rtn->protocol != addr->protocol) {
        return FALSE;
    }

    if (TRUE == irp_match_rtn_ip_out(rtn, addr)) {
        return TRUE;
    }

    if (TRUE == irp_match_rtn_ip_in(rtn, addr)) {
        return TRUE;
    }

    return FALSE;
}

static inline BOOL_T irp_match_threshold_opt(IN IRP_THRESHOLD_OPT_S *opt, IRP_PKT_INFO_S *pkt_info)
{
    if (! opt->enabled) {
        return TRUE;
    }

    /* TODO: 检测Threshold */

    return TRUE;
}

static inline BOOL_T irp_match_tcp_opt(IN IRP_TCP_OPT_S *opt, IRP_PKT_INFO_S *pkt_info)
{
    if (! opt->enabled) {
        return TRUE;
    }

    /* TODO: 处理Tcp选项 */

    return TRUE;
}

static inline BOOL_T irp_match_pcre(IN IRP_PCRE_S *opt, IRP_PKT_INFO_S *pkt_info)
{
    int rc;
    int ovector[3];

    if (! opt->pcre) {
        return TRUE;
    }

    if((rc=pcre_exec(opt->pcre, NULL, (const char*)pkt_info->l4_payload,
                    pkt_info->l4_payload_len, 0, 0, ovector, 3)) <=0) {
        return FALSE;
    }

    return TRUE;
}

static UCHAR * irp_match_content_opt(IRP_CONTENT_OPT_S *opt, void *data, int data_len, UCHAR *prev_match_content)
{
    UCHAR *find_content;
    LDATA_S d;

    d.pucData = data;
    d.uiLen = data_len;

    if (opt->offset) {
        if (opt->offset >= data_len) {
            return NULL;
        }
        d.pucData += opt->offset;
        d.uiLen -= opt->offset;
    }

    if (opt->depth) {
        d.uiLen = MIN(d.uiLen, opt->depth);
    }

    if (opt->nocase) {
        find_content = LDATA_Stristr(&d, &opt->content);
    } else {
        find_content = LDATA_Strstr(&d, &opt->content);
    }

    if (! find_content) {
        return NULL;
    }

    int len = find_content - prev_match_content;

    if (opt->within) {
        if (len > opt->within) {
            return NULL;
        }
    }

    if (opt->distance) {
        if (len < opt->distance) {
            return NULL;
        }
    }

    /* TODO: 其他选线处理  */

    return find_content;
}

static BOOL_T irp_match_otn(IRP_OTN_S *otn, IRP_PKT_INFO_S *pkt_info)
{
    IRP_CONTENT_OPT_S *opt;
    UCHAR * context_matched = NULL;

    if (FALSE == irp_match_threshold_opt(&otn->threshold_opt, pkt_info)) {
        return FALSE;
    }

    if (FALSE == irp_match_tcp_opt(&otn->tcp_opt, pkt_info)) {
        return FALSE;
    }

    DLL_SCAN(&otn->content_opt_list, opt) {
        context_matched = irp_match_content_opt(opt, pkt_info->l4_payload, pkt_info->l4_payload_len, context_matched);
        if (! context_matched) {
            return FALSE;
        }
    }

    if (FALSE == irp_match_pcre(&otn->pcre_opt, pkt_info)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL_T irp_match_rule(IRP_OTN_S *otn, IRP_PKT_INFO_S *pkt_info)
{
    if (TRUE != irp_match_rtn(otn->rtn, pkt_info->addr)) {
        return FALSE;
    }

    if (TRUE != irp_match_otn(otn, pkt_info)) {
        return FALSE;
    }

    return TRUE;
}

static int irp_ac_matched(void *mlist, int offset, void *user_data)
{
    IRP_MATCH_UD_S *ud = user_data;
    ACSMX_PATTERN_S* acmatch = mlist;
    IRP_OTN_S *otn;

    for(; acmatch; acmatch=acmatch->next) {
        otn = acmatch->udata->id;    

        if (FALSE == irp_match_rule(otn, ud->pkt_info)) {
            return 0;
        }

        ud->func(otn, ud->ud);
    }

    return 0;
}

int IRP_Match(IRP_CTRL_S *ctrl, IRP_PKT_INFO_S *info, PF_IRP_MATCH_FUNC func, void *ud)
{
    int state = 0;
    IRP_MATCH_UD_S user_data;

    user_data.pkt_info = info;
    user_data.func = func;
    user_data.ud = ud;

    ACSMX_Search(ctrl->ac, info->l4_payload, info->l4_payload_len, irp_ac_matched, &user_data, &state);

    return 0;
}

