/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/exec_utl.h"
#include "utl/map_utl.h"
#include "utl/mybpf_prog.h"
#include "utl/mybpf_loader.h"
#include "utl/mybpf_runtime.h"
#include "utl/mybpf_hookpoint.h"
#include "comp/comp_precver.h"
#include "comp/comp_event_hub.h"
#include "../h/ulcapp_cfg_lock.h"

static MYBPF_RUNTIME_S g_ulcapp_runtime;

static int ulcapp_precver_xdp_pkt_input(void *ob, void *data)
{
    PRECVER_PKT_S *pkt = data;
    MYBPF_XDP_BUFF_S xdp_buf = {0};

    xdp_buf.data = pkt->data;
    xdp_buf.data_end = pkt->data + pkt->data_len;

    MYBPF_XdpInput(&g_ulcapp_runtime.xdp_list, &xdp_buf);

    return 0;
}

static int ulcapp_load_file(char *filename, char *instance)
{
    MYBPF_LOADER_PARAM_S p = {0};

    if (MYBPF_LoaderGet(&g_ulcapp_runtime, instance)) {
        EXEC_OutInfo("Instance %s already loaded \r\n", instance);
        RETURN(BS_ALREADY_EXIST);
    }

    p.filename = filename;
    p.instance = instance;
    p.flag |= MYBPF_LOADER_FLAG_AUTO_ATTACH;

    return MYBPF_LoaderLoad(&g_ulcapp_runtime, &p);
}

static int _ulcapp_replace_file(char *filename, char *instance, UINT keep_map)
{
    MYBPF_LOADER_PARAM_S p = {0};

    if (! MYBPF_LoaderGet(&g_ulcapp_runtime, instance)) {
        EXEC_OutInfo("Instance %s is not exist \r\n", instance);
        RETURN(BS_NOT_FOUND);
    }

    p.filename = filename;
    p.instance = instance;
    p.flag |= MYBPF_LOADER_FLAG_AUTO_ATTACH;
    if (keep_map) {
        p.flag |= MYBPF_LOADER_FLAG_KEEP_MAP;
    }

    return MYBPF_LoaderLoad(&g_ulcapp_runtime, &p);
}

int ULCAPP_RuntimeInit(void)
{
    MYBPF_RuntimeInit(&g_ulcapp_runtime);
    EHUB_Reg(EHUB_EV_PRECVER_PKT, "ulc", ulcapp_precver_xdp_pkt_input, NULL);
    return 0;
}

int ULCAPP_LoadFile(char *filename, char *instance)
{
    ULCAPP_CfgLock();
    int ret = ulcapp_load_file(filename, instance);
    ULCAPP_CfgUnlock();

    return ret;
}

int ULCAPP_ReplaceFile(char *filename, char *instance, UINT keep_map)
{
    ULCAPP_CfgLock();
    int ret = _ulcapp_replace_file(filename, instance, keep_map);
    ULCAPP_CfgUnlock();

    return ret;
}

int ULCAPP_UnloadInstance(char *instance)
{
    ULCAPP_CfgLock();
    MYBPF_LoaderUnload(&g_ulcapp_runtime, instance);
    ULCAPP_CfgUnlock();
    return 0;
}

int ULCAPP_RuntimeSave(HANDLE hFile)
{
    void *iter = NULL;
    MYBPF_LOADER_PARAM_S *p;
    MYBPF_LOADER_NODE_S *n;

    ULCAPP_CfgLock();
    while ((n = MYBPF_LoaderGetNext(&g_ulcapp_runtime, &iter))) {
        p = &n->param;
        CMD_EXP_OutputCmd(hFile, "load %s file %s", p->instance, p->filename);
    }
    ULCAPP_CfgUnlock();

    return 0;
}


