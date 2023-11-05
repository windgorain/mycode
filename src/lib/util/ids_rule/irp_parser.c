/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/txt_utl.h"
#include "utl/ip_utl.h"
#include "utl/ac_smx.h"
#include "utl/ip_string.h"
#include "utl/ip_protocol.h"
#include "utl/socket_utl.h"
#include "utl/kv_utl.h"
#include "utl/tcp_utl.h"
#include "utl/irp_utl.h"
#include "pcre.h"
#include "irp_def.h"

typedef struct {
    char *type;
    char *protocol;
    char *sip;
    char *sport;
    char *dir;
    char *dip;
    char *dport;
    KV_HANDLE kv;
}IRP_STRING_S;

typedef int (*PF_IRP_OPT_KV_FUNC)(IRP_OTN_S *otn, char *value);

typedef struct {
    char *pattern;
    PF_IRP_OPT_KV_FUNC func;
}IRP_OPT_KV_FUNC_S;

static int irp_parse_string(char *str, OUT IRP_STRING_S *node)
{
    char *toks[7];
    UINT count;

    count = TXT_StrToToken(str, " \t", toks, 7);
    if (count < 7) {
        RETURN(BS_ERR);
    }

    node->type = toks[0];
    node->protocol = toks[1];
    node->sip = toks[2];
    node->sport = toks[3];
    node->dir = toks[4];
    node->dip = toks[5];
    node->dport = toks[6];

    char *opt = toks[6];
    opt = opt + strlen(opt);
    opt ++;
    opt = TXT_Strim(opt);
    opt = TXT_StrimString(opt, "()");

    LSTR_S lstr;
    lstr.pcData = opt;
    lstr.uiLen = strlen(opt);

    if (0 != KV_Parse(node->kv, &lstr, ';', ':')) {
        RETURN(BS_ERR);
    }

    return 0;
}

static IRP_RTN_S * irp_find_rtn(IRP_CTRL_S *ctrl, IRP_RTN_S *tofind) 
{
    IRP_RTN_S *node;

    DLL_SCAN(&ctrl->rtn_list, node) {
        if (memcmp(node, tofind, sizeof(IRP_RTN_S)) == 0) {
            return node;
        }
    }

    return NULL;
}

static void irp_opt_judge_fast_opt(IRP_OTN_S *otn, IRP_CONTENT_OPT_S *opt)
{
    if (otn->fast_opt == NULL) {
        otn->fast_opt = opt;
        return;
    }

    
    if (otn->fast_opt->fast_pattern) {
        return;
    }

    
    if (otn->fast_opt->content.len < opt->content.len) {
        otn->fast_opt = opt;
    }

    return;
}

static int irp_opt_kv_msg(IRP_OTN_S *otn, char *value)
{
    otn->msg = TXT_Strdup(value);
    return 0;
}

static int irp_opt_kv_sid(IRP_OTN_S *otn, char *value)
{
    otn->sid = TXT_Str2Ui(value);
    return 0;
}

static int irp_opt_kv_gid(IRP_OTN_S *otn, char *value)
{
    otn->gid = TXT_Str2Ui(value);
    return 0;
}

static int irp_opt_kv_flags(IRP_OTN_S *otn, char *value)
{
    IRP_TCP_OPT_S *tcp_opt = &otn->tcp_opt;
    tcp_opt->flags = TCP_String2Flag(value);
    return 0;
}

static int irp_opt_kv_flow(IRP_OTN_S *otn, char *value)
{
    char *flag[32];
    IRP_TCP_OPT_S *tcp_opt = &otn->tcp_opt;
    char *str;

    int num = TXT_StrToToken(value, ",", flag, 32);
    int i;

    for (i=0; i<num; i++) {
        str = TXT_Strim(flag[i]);
        if (strcmp(str, "to_client") == 0) {
            tcp_opt->to_client = 1;
        } else if (strcmp(str, "to_server") == 0) {
            tcp_opt->to_server = 1;
        } else if (strcmp(str, "from_client") == 0) {
            tcp_opt->from_client = 1;
        } else if (strcmp(str, "from_server") == 0) {
            tcp_opt->from_server = 1;
        } else if (strcmp(str, "established") == 0) {
            tcp_opt->established = 1;
        } else if (strcmp(str, "stateless") == 0) {
            tcp_opt->stateless = 1;
        } else if (strcmp(str, "no_stream") == 0) {
            tcp_opt->no_stream = 1;
        } else if (strcmp(str, "only_stream") == 0) {
            tcp_opt->only_stream = 1;
        } else {
            BS_DBGASSERT(0);
        }
    }

    return 0;
}

static int irp_opt_kv_pcre(IRP_OTN_S *otn, char *value)
{
    int erroroffset;
    const char* error;

    otn->pcre_opt.pcre = pcre_compile(value, 0, &error, &erroroffset, NULL);
    if (! otn->pcre_opt.pcre) {
        RETURN(BS_NO_MEMORY);
    }
    return 0;
}

static int irp_opt_kv_content(IRP_OTN_S *otn, char *value)
{
    IRP_CONTENT_OPT_S * opt = MEM_ZMalloc(sizeof(IRP_CONTENT_OPT_S));
    if (! opt) {
        RETURN(BS_NO_MEMORY);
    }

    opt->content.data = (void*)TXT_Strdup(value);
    if (! opt->content.data) {
        MEM_Free(opt);
        RETURN(BS_NO_MEMORY);
    }

    opt->content.len = strlen(value);

    irp_opt_judge_fast_opt(otn, opt);

    DLL_ADD(&otn->content_opt_list, &opt->link_node);

    return 0;
}

static int irp_opt_kv_uri_content(IRP_OTN_S *otn, char *value)
{
    int ret = irp_opt_kv_content(otn, value);
    if (ret != 0) {
        return ret;
    }

    IRP_CONTENT_OPT_S *opt = DLL_LAST(&otn->content_opt_list);
    if (! opt) {
        RETURN(BS_ERR);
    }

    opt->content_type = IRP_CONTENT_TYPE_URI_CONTENT;

    return 0;
}

static int irp_opt_kv_nocase(IRP_OTN_S *otn, char *value)
{
    IRP_CONTENT_OPT_S *opt = DLL_LAST(&otn->content_opt_list);
    if (! opt) {
        RETURN(BS_ERR);
    }
    opt->nocase = 1;
    return 0;
}

static int irp_opt_kv_rawbytes(IRP_OTN_S *otn, char *value)
{
    IRP_CONTENT_OPT_S *opt = DLL_LAST(&otn->content_opt_list);
    if (! opt) {
        RETURN(BS_ERR);
    }
    opt->content_type = IRP_CONTENT_TYPE_RAWBYTES;
    return 0;
}

static int irp_opt_kv_depth(IRP_OTN_S *otn, char *value)
{
    IRP_CONTENT_OPT_S *opt = DLL_LAST(&otn->content_opt_list);
    if (! opt) {
        RETURN(BS_ERR);
    }
    opt->depth = TXT_Str2Ui(value);
    return 0;
}

static int irp_opt_kv_offset(IRP_OTN_S *otn, char *value)
{
    IRP_CONTENT_OPT_S *opt = DLL_LAST(&otn->content_opt_list);
    if (! opt) {
        RETURN(BS_ERR);
    }
    opt->offset = TXT_Str2Ui(value);
    return 0;
}

static int irp_opt_kv_distance(IRP_OTN_S *otn, char *value)
{
    IRP_CONTENT_OPT_S *opt = DLL_LAST(&otn->content_opt_list);
    if (! opt) {
        RETURN(BS_ERR);
    }
    opt->distance = TXT_Str2Ui(value);
    return 0;
}

static int irp_opt_kv_within(IRP_OTN_S *otn, char *value)
{
    IRP_CONTENT_OPT_S *opt = DLL_LAST(&otn->content_opt_list);
    if (! opt) {
        RETURN(BS_ERR);
    }
    opt->within = TXT_Str2Ui(value);
    return 0;
}

static int irp_opt_kv_fast_pattern(IRP_OTN_S *otn, char *value)
{
    IRP_CONTENT_OPT_S *opt = DLL_LAST(&otn->content_opt_list);
    if (! opt) {
        RETURN(BS_ERR);
    }
    opt->fast_pattern = 1;
    otn->fast_opt = opt;
    return 0;
}

static int irp_opt_kv_http_uri(IRP_OTN_S *otn, char *value)
{
    IRP_CONTENT_OPT_S *opt = DLL_LAST(&otn->content_opt_list);
    if (! opt) {
        RETURN(BS_ERR);
    }
    opt->content_type = IRP_CONTENT_TYPE_HTTP_URI;
    return 0;
}

static int irp_opt_kv_http_raw_uri(IRP_OTN_S *otn, char *value)
{
    IRP_CONTENT_OPT_S *opt = DLL_LAST(&otn->content_opt_list);
    if (! opt) {
        RETURN(BS_ERR);
    }
    opt->content_type = IRP_CONTENT_TYPE_HTTP_RAW_URI;
    return 0;
}

static int irp_opt_kv_http_state_code(IRP_OTN_S *otn, char *value)
{
    IRP_CONTENT_OPT_S *opt = DLL_LAST(&otn->content_opt_list);
    if (! opt) {
        RETURN(BS_ERR);
    }
    opt->content_type = IRP_CONTENT_TYPE_HTTP_STATE_CODE;
    return 0;
}

static int irp_opt_kv_http_state_msg(IRP_OTN_S *otn, char *value)
{
    IRP_CONTENT_OPT_S *opt = DLL_LAST(&otn->content_opt_list);
    if (! opt) {
        RETURN(BS_ERR);
    }
    opt->content_type = IRP_CONTENT_TYPE_HTTP_STATE_MSG;
    return 0;
}

static int irp_opt_kv_http_header(IRP_OTN_S *otn, char *value)
{
    IRP_CONTENT_OPT_S *opt = DLL_LAST(&otn->content_opt_list);
    if (! opt) {
        RETURN(BS_ERR);
    }
    opt->content_type = IRP_CONTENT_TYPE_HTTP_HEADER;
    return 0;
}

static int irp_opt_kv_http_raw_header(IRP_OTN_S *otn, char *value)
{
    IRP_CONTENT_OPT_S *opt = DLL_LAST(&otn->content_opt_list);
    if (! opt) {
        RETURN(BS_ERR);
    }
    opt->content_type = IRP_CONTENT_TYPE_HTTP_RAW_HEADER;
    return 0;
}

static int irp_opt_kv_http_client_body(IRP_OTN_S *otn, char *value)
{
    IRP_CONTENT_OPT_S *opt = DLL_LAST(&otn->content_opt_list);
    if (! opt) {
        RETURN(BS_ERR);
    }
    opt->content_type = IRP_CONTENT_TYPE_HTTP_CLIENT_BODY;
    return 0;
}

static int irp_opt_kv_http_cookie(IRP_OTN_S *otn, char *value)
{
    IRP_CONTENT_OPT_S *opt = DLL_LAST(&otn->content_opt_list);
    if (! opt) {
        RETURN(BS_ERR);
    }
    opt->content_type = IRP_CONTENT_TYPE_HTTP_COOKIE;
    return 0;
}

static int irp_opt_kv_http_raw_cookie(IRP_OTN_S *otn, char *value)
{
    IRP_CONTENT_OPT_S *opt = DLL_LAST(&otn->content_opt_list);
    if (! opt) {
        RETURN(BS_ERR);
    }
    opt->content_type = IRP_CONTENT_TYPE_HTTP_RAW_COOKIE;
    return 0;
}


static int irp_opt_kv_threshold(IRP_OTN_S *otn, char *value)
{
    IRP_THRESHOLD_OPT_S *threshold_opt = &otn->threshold_opt;
    char *flag[32];
    char *str;
    int i;

    int num = TXT_StrToToken(value, ",", flag, 32);

    for (i=0; i<num; i++) {
        str = TXT_Strim(flag[i]);
        if (strstr(str, "type")) {
            if (strstr(str, "both")) {
                threshold_opt->type_limit = 1;
                threshold_opt->type_threshold = 1;
            } else if (strstr(str, "limit")) {
                threshold_opt->type_limit = 1;
            } else if (strstr(str, "threshold")) {
                threshold_opt->type_threshold = 1;
            } else {
                BS_DBGASSERT(0);
            }
        } else if (strstr(str, "track")) {
            if (strstr(str, "by_dst")) {
                threshold_opt->track_by_dst = 1;
            } else if (strstr(str, "by_src")) {
                threshold_opt->track_by_src = 1;
            } else {
                BS_DBGASSERT(0);
            }
        } else if (strstr(str, "count")) {
            threshold_opt->count = TXT_Str2Ui(str + 6);
        } else if (strstr(str, "seconds")) {
            threshold_opt->seconds = TXT_Str2Ui(str + sizeof("seconds"));
        } else {
            BS_DBGASSERT(0);
        }
    }

    return 0;
}

static IRP_OPT_KV_FUNC_S g_irp_kv_funcs[] = {
    {"msg", irp_opt_kv_msg},

    {"sid", irp_opt_kv_sid},
    {"gid", irp_opt_kv_gid},

    
    {"flags", irp_opt_kv_flags},
    {"flow", irp_opt_kv_flow},

    
    {"content", irp_opt_kv_content},
    {"uricontent", irp_opt_kv_uri_content},
    {"nocase", irp_opt_kv_nocase},
    {"rawbytes", irp_opt_kv_rawbytes},
    {"depth", irp_opt_kv_depth},
    {"offset", irp_opt_kv_offset},
    {"distance", irp_opt_kv_distance},
    {"within", irp_opt_kv_within},
    {"fast_pattern", irp_opt_kv_fast_pattern},
    {"http_client_body", irp_opt_kv_http_client_body},
    {"http_cookie", irp_opt_kv_http_cookie},
    {"http_raw_cookie", irp_opt_kv_http_raw_cookie},
    {"http_header", irp_opt_kv_http_header},
    {"http_raw_header", irp_opt_kv_http_raw_header},
    {"http_uri", irp_opt_kv_http_uri},
    {"http_raw_uri", irp_opt_kv_http_raw_uri},
    {"http_stat_code", irp_opt_kv_http_state_code},
    {"http_stat_msg", irp_opt_kv_http_state_msg},

    
    {"pcre", irp_opt_kv_pcre},

    
    {"threshold", irp_opt_kv_threshold},

    
    {"metadata", NULL},
    {"reference", NULL},
    {"classtype", NULL},
    {"rev", NULL},

    {NULL, NULL}
};

static IRP_OPT_KV_FUNC_S * irp_find_opt_kv_func(KV_S *ele)
{
    IRP_OPT_KV_FUNC_S *f = &g_irp_kv_funcs[0];

    while (f->pattern) {
        if (0 == strcmp(f->pattern, ele->pcKey)) {
            return f;
        }
        f ++;
    }

    return NULL;
}

static int irp_process_opt_kv(IRP_OTN_S *otn, KV_S *ele)
{
    IRP_OPT_KV_FUNC_S *f = irp_find_opt_kv_func(ele);
    char *value = ele->pcValue;

    if (f == NULL) {
        printf("Not support the option %s \r\n", ele->pcKey);
        RETURN(BS_NOT_SUPPORT);
    }

    if (f->func == NULL) {
        RETURN(BS_NOT_SUPPORT);
    }

    value = TXT_StrimString(value, " \t\"\'");

    return f->func(otn, value);
}

static int irp_process_opts(IRP_CTRL_S *ctrl, IRP_OTN_S *otn, KV_HANDLE kv)
{
    KV_S *ele = NULL;

    while((ele = KV_GetNext(kv, ele))) {
        irp_process_opt_kv(otn, ele);
    }

    return 0;
}

static void irp_free_opt(IRP_CONTENT_OPT_S *opt)
{
    if (opt->content.data) {
        MEM_Free(opt->content.data);
    }

    MEM_Free(opt);
}

static void irp_free_pcre(IRP_PCRE_S *pcre_opt)
{
    if (pcre_opt->pcre) {
        free(pcre_opt->pcre);
        pcre_opt->pcre = NULL;
    }
}

static void irp_free_otn(IRP_OTN_S *otn)
{
    IRP_CONTENT_OPT_S *opt, *opttmp;

    DLL_SAFE_SCAN(&otn->content_opt_list, opt, opttmp) {
        DLL_DEL(&otn->content_opt_list, &opt->link_node);
        irp_free_opt(opt);
    }

    irp_free_pcre(&otn->pcre_opt);

    MEM_Free(otn);
}

static int irp_get_action_by_string(char *type_str)
{
    if (strcmp(type_str, "alert") == 0) {
        return IRP_RULE_ACTION_ALERT;
    }

    if (strcmp(type_str, "log") == 0) {
        return IRP_RULE_ACTION_LOG;
    }

    if (strcmp(type_str, "pass") == 0) {
        return IRP_RULE_ACTION_PASS;
    }

    BS_DBGASSERT(0); 

    return IRP_RULE_ACTION_PASS;
}

static int irp_add_otn(IRP_CTRL_S *ctrl, IRP_RTN_S *rtn, IRP_STRING_S *str_node)
{
    IRP_OTN_S *otn = MEM_ZMalloc(sizeof(IRP_OTN_S));

    if (! otn) {
        RETURN(BS_NO_MEMORY);
    }

    DLL_INIT(&otn->content_opt_list);
    otn->rtn = rtn;
    otn->action = irp_get_action_by_string(str_node->type);

    if (0 != irp_process_opts(ctrl, otn, str_node->kv)) {
        irp_free_otn(otn);
        RETURN(BS_NO_MEMORY);
    }

    DLL_ADD(&rtn->otn_list, &otn->link_node);

    return 0;
}

static IRP_RTN_S * irp_add_rtn(IRP_CTRL_S *ctrl, IRP_STRING_S *str_node)
{
    IRP_RTN_S to_find;

    memset(&to_find, 0, sizeof(to_find));

    to_find.protocol = IPProtocol_GetByName(str_node->protocol);

    

    if (stricmp(str_node->sip, "ANY") != 0) {
        if (0 != IPString_ParseIpMask(str_node->sip, &to_find.sip)) {
            return NULL;
        }
    }

    if (stricmp(str_node->dip, "ANY") != 0) {
        if (0 != IPString_ParseIpMask(str_node->dip, &to_find.dip)) {
            return NULL;
        }
    }

    if (stricmp(str_node->sport, "ANY") != 0) {
        to_find.sport = TXT_Str2Ui(str_node->sport);
    }

    if (stricmp(str_node->dport, "ANY") != 0) {
        to_find.dport = TXT_Str2Ui(str_node->dport);
    }

    if (strcmp(str_node->dir, "->") == 0) {
        to_find.dir_out = 1;
    } else if (strcmp(str_node->dir, "<>") == 0) {
        to_find.dir_out = 1;
        to_find.dir_in = 1;
    }

    IRP_RTN_S *rtn = irp_find_rtn(ctrl, &to_find);
    if (! rtn) {
        rtn = MEM_Dup(&to_find, sizeof(IRP_RTN_S));
        if (! rtn) {
            return NULL;
        }
        DLL_INIT(&rtn->otn_list);
        DLL_ADD(&ctrl->rtn_list, &rtn->link_node);
    }

    return rtn;
}

static int irp_add_rule(IRP_CTRL_S *ctrl, IRP_STRING_S *str_node)
{
    IRP_RTN_S *rtn = irp_add_rtn(ctrl, str_node);
    if (! rtn) {
        RETURN(BS_ERR);
    }

    return irp_add_otn(ctrl, rtn, str_node);
}

int IRP_AddRule(IRP_CTRL_S *ctrl, char *rule)
{
    IRP_STRING_S str_node;
    int ret;

    rule = TXT_Strim(rule);

    memset(&str_node, 0, sizeof(str_node));
    str_node.kv = KV_Create(NULL);
    if (! str_node.kv) {
        RETURN(BS_NO_MEMORY);
    }

    ret = irp_parse_string(rule, &str_node);
    if (0 == ret) {
        ret = irp_add_rule(ctrl, &str_node);
    }

    KV_Destory(str_node.kv);
    
    return ret;
}

int IRP_LoadRuleByFile(IRP_CTRL_S *ctrl, char *filename)
{
    FILE *fp;
    char *line;
    char buf[2048];

    fp = fopen(filename, "rb+");
    if (! fp) {
        RETURN(BS_CAN_NOT_OPEN);
    }

    while ((line = fgets(buf, sizeof(buf), fp)) != NULL) {
        IRP_AddRule(ctrl, line);
    }

    fclose(fp);

    return 0;
}

int _IRP_DelAllRule(IRP_CTRL_S *ctrl)
{
    IRP_RTN_S *rtn, *rtn_tmp;
    IRP_OTN_S *otn, *otn_tmp;

    DLL_SAFE_SCAN(&ctrl->rtn_list, rtn, rtn_tmp) {
        DLL_SAFE_SCAN(&rtn->otn_list, otn, otn_tmp) {
            DLL_DEL(&rtn->otn_list, otn);
            irp_free_otn(otn);
        }
        DLL_DEL(&ctrl->rtn_list, rtn);
        MEM_Free(rtn);
    }

    return 0;
}


