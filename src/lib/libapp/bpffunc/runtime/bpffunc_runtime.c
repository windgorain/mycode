/*****************************************************************************
*   Created by LiXingang, Copyright LiXingang
*   Description: 
*
*****************************************************************************/
#include "bs.h"
#include "utl/idfunc_utl.h"
#include "utl/exec_utl.h"
#include "utl/mybpf_prog.h"

static MYBPF_RUNTIME_S g_bpffunc_runtime;
static IDFUNC_S *g_bpffunc_ctx;

int BPFFUNC_RuntimeInit(void)
{
    int ret;

    ret = MYBPF_RuntimeInit(&g_bpffunc_runtime, 1024);
    if (ret < 0) {
        return ret;
    }

    g_bpffunc_ctx = IDFUNC_Create(1024);
    if (! g_bpffunc_ctx) {
        RETURN(BS_NO_MEMORY);
    }

    return IDFUNC_Load(&g_bpffunc_runtime, g_bpffunc_ctx, "conf_dft/plug/bpffunc/bpffunc_config.ini");
}

PLUG_API int BPFFUNC_ShowProg(void)
{
    MYBPF_PROG_ShowProg(&g_bpffunc_runtime, EXEC_OutInfo);
    return 0;
}

PLUG_API int BPFFUNC_RunProg(int argc, char **argv)
{
    UINT64 bpf_ret;
    UINT id;

    id = TXT_Str2Ui(argv[2]);

    IDFUNC_Call(g_bpffunc_ctx, id, &bpf_ret, 0, 0, 0, 0, 0);
    return 0;
}

PLUG_API IDFUNC_S * BPFFUNC_GetCtx(void)
{
    return g_bpffunc_ctx;
}


