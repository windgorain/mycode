/*****************************************************************************
*   Created by LiXingang, Copyright LiXingang
*   Description: 
*
*****************************************************************************/
#include "bs.h"
#include "utl/bfunc_utl.h"
#include "utl/exec_utl.h"
#include "utl/mybpf_loader.h"
#include "utl/mybpf_prog.h"
#include "comp/comp_bpffunc.h"

#define BPFFUNC_MAX 1024

static MYBPF_RUNTIME_S g_bpffunc_runtime;
static BFUNC_S *g_bpffunc_ctx;

int BPFFUNC_RuntimeInit(void)
{
    int ret;

    ret = MYBPF_RuntimeInit(&g_bpffunc_runtime);
    if (ret < 0) {
        return ret;
    }

    g_bpffunc_ctx = BFUNC_Create(&g_bpffunc_runtime, BPFFUNC_MAX);
    if (! g_bpffunc_ctx) {
        RETURN(BS_NO_MEMORY);
    }

    return BFUNC_Load(g_bpffunc_ctx, "conf_dft/plug/bpffunc/bpffunc_config.ini");
}

PLUG_API int BPFFUNC_ShowProg(void)
{
    int i;
    BFUNC_NODE_S *node;
    MYBPF_PROG_NODE_S *prog;
    MYBPF_LOADER_NODE_S *loader;

    for (i=0; i<BPFFUNC_MAX; i++) {
        node = BFUNC_Get(g_bpffunc_ctx, i);
        if ((! node) || (! node->func)){
            continue;
        }

        prog = node->prog;
        if (! prog) {
            continue;
        }
        loader = prog->loader_node;
        EXEC_OutInfo("id:%d, xlated:%u, sec:%s, name:%s, instance:%s, file:%s \r\n",
                i, prog->insn_len, prog->sec_name, prog->prog_name,
                loader->param.instance, loader->param.filename);
    }

    return 0;
}

PLUG_API int BPFFUNC_RunProg(int argc, char **argv)
{
    UINT64 bpf_ret;
    UINT id;

    id = TXT_Str2Ui(argv[2]);

    BFUNC_Call(g_bpffunc_ctx, id, &bpf_ret, 0, 0, 0, 0, 0);

    return 0;
}

PLUG_API BFUNC_S * BPFFUNC_GetCtx(void)
{
    return g_bpffunc_ctx;
}

PLUG_API int BPFFUNC_Call(UINT id, UINT64 *bpf_ret,
        UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5)
{
    return BFUNC_Call(g_bpffunc_ctx, id, bpf_ret, p1, p2, p3, p4, p5);
}

