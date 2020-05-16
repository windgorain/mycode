/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/eth_utl.h"
#include "utl/exec_utl.h"
#include "utl/txt_utl.h"
#include "utl/ip_utl.h"
#include "utl/tcp_utl.h"
#include "utl/udp_utl.h"
#include "utl/ubpf_utl.h"
#include "utl/http_lib.h"
#include "utl/http_monitor.h"
#include "comp/comp_pissuer.h"

#include "../h/pissuer_func.h"

#define PISSUER_VIEW_NAME "pissuer-view"


static DLL_HEAD_S g_pissuer_pkt_hooks[PISSUER_EV_MAX];

PLUG_API int pissue_CmdShow(UINT argc, char **argv, void *env)
{
    int i;
    PISSUER_EV_OB_S *ob;

    EXEC_OutInfo(" name    Enable filter\r\n");
    EXEC_OutInfo(" ------------------------------------------------------\r\n");
    for (i=0; i<PISSUER_EV_MAX; i++) {
        DLL_SCAN(&g_pissuer_pkt_hooks[i], ob) {
            EXEC_OutInfo(" %-16s %-8u %s\r\n",
                    ob->ob_name, ob->enabled, ob->bpf_string);
        }
    }

    return 0;
}

static inline int pissuer_bpf_exec(PISSUER_EV_OB_S* ob, PISSUER_PKT_S* pkt)
{
    ubpf_jit_fn jit_func = ob->jit.jit_func;

    if (unlikely(NULL != jit_func)) {
        return jit_func(pkt->recver_pkt->data, pkt->recver_pkt->data_len);
    }

    return 1;
}

static int pissuer_ob_pkt_disable(UINT argc, char **argv, void *env)
{
    PISSUER_EV_OB_S *ob;

    ob = CmdExp_GetParam(env);
    if (! ob) {
        RETURN(BS_ERR);
    }

    ob->enabled = 0;

    return 0;
}

static int pissuer_ob_pkt_enable(UINT argc, char **argv, void *env)
{
    PISSUER_EV_OB_S *ob;

    ob = CmdExp_GetParam(env);
    if (! ob) {
        RETURN(BS_ERR);
    }

    ob->enabled = 1;

    return 0;
}

static int pissuer_pkt_filter(UINT argc, char **argv, void *env)
{
    PISSUER_EV_OB_S *ob;
    char *filter = NULL;

    ob = CmdExp_GetParam(env);
    if (! ob) {
        RETURN(BS_ERR);
    }

    if (argc >= 4) {
        filter = argv[3];
    }

    if (0 != PIssuer_SetFilter(ob, filter)) {
        EXEC_OutString("Can't compile bpf");
    }

    return 0;
}

PLUG_API int PIssuer_Reg(int point, PISSUER_EV_OB_S *ob)
{
    char cmd[256];

    BS_DBGASSERT(point < PISSUER_EV_MAX);

    if (PIssuer_Find(ob->ob_name)) {
        RETURN(BS_ALREADY_EXIST);
    }

    snprintf(cmd, sizeof(cmd), "ob %s enable", ob->ob_name);
    CMD_EXP_RegCmdSimple(PISSUER_VIEW_NAME, cmd, pissuer_ob_pkt_enable, ob);
    snprintf(cmd, sizeof(cmd), "no ob %s enable", ob->ob_name);
    CMD_EXP_RegCmdSimple(PISSUER_VIEW_NAME, cmd, pissuer_ob_pkt_disable, ob);
    snprintf(cmd, sizeof(cmd),
            "ob %s filter [%%STRING<1-511>(BPF filter. Eg: \'tcp port 80\')]",
            ob->ob_name);
    CMD_EXP_RegCmdSimple(PISSUER_VIEW_NAME, cmd, pissuer_pkt_filter, ob);

    DLL_ADD(&g_pissuer_pkt_hooks[point], &ob->link_node);

    return 0;
}

PLUG_API void PIssuer_UnReg(PISSUER_EV_OB_S *ob)
{
    char cmd[256];

    snprintf(cmd, sizeof(cmd), "ob %s disable", ob->ob_name);
    CMD_EXP_UnregCmdSimple(PISSUER_VIEW_NAME, cmd);
    snprintf(cmd, sizeof(cmd), "ob %s enable", ob->ob_name);
    CMD_EXP_UnregCmdSimple(PISSUER_VIEW_NAME, cmd);
    snprintf(cmd, sizeof(cmd), "ob %s filter [%%STRING<1-511>]", ob->ob_name);
    CMD_EXP_UnregCmdSimple(PISSUER_VIEW_NAME, cmd);

    DLL_DelIfInList(&ob->link_node);
}

PLUG_API PISSUER_EV_OB_S * PIssuer_Find(char *ob_name)
{
    int i; 
    PISSUER_EV_OB_S *ob;

    for (i=0; i<PISSUER_EV_MAX; i++) {
        DLL_SCAN(&g_pissuer_pkt_hooks[i], ob) {
            if (0 == strcmp(ob_name, ob->ob_name)) {
                return ob;
            }
        }
    }

    return NULL;
}

PLUG_API int PIssuer_SetFilter(PISSUER_EV_OB_S *ob, char *cbpf_string)
{
    UBPF_JIT_S jit = {0};

    if (cbpf_string) {
        if (BS_OK != UBPF_S2j(DLT_EN10MB, cbpf_string, &ob->jit)) {
            RETURN(BS_ERR);
        }
    }

    /* 释放前一次配置所申请的资源 */
    if(ob->jit.vm) {
        UBPF_Destroy(ob->jit.vm);
    }

    ob->jit = jit;

    ob->bpf_string[0] = 0;
    if (cbpf_string) {
        strlcpy(ob->bpf_string, cbpf_string, sizeof(ob->bpf_string));
    }

    return 0;
}

PLUG_API void PIssuer_Publish(int point, PISSUER_PKT_S *pkt)
{
    PISSUER_EV_OB_S *ob;

    BS_DBGASSERT(point < PISSUER_EV_MAX);

    DLL_SCAN(&g_pissuer_pkt_hooks[point], ob) {
        if (! ob->enabled) {
            continue;
        }
        if(! ob->pkt_func) {
            continue;
        }
        if(0 == pissuer_bpf_exec(ob, pkt)) {
            continue;
        }

        ob->pkt_func(pkt);
    }
}

int PIssuer_Core_Init()
{
    int i;

    for (i=0; i<PISSUER_EV_MAX; i++) {
        DLL_INIT(&g_pissuer_pkt_hooks[i]);
    }

    return 0;
}

