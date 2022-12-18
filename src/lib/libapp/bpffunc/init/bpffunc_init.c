/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
*************************************************/
#include "bs.h"
#include "utl/idfunc_utl.h"

static MYBPF_RUNTIME_S g_bpffunc_runtime;
static IDFUNC_S *g_bpffunc_ctx;

int BPFFUNC_Init(void)
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

    return IDFUNC_Load(&g_bpffunc_runtime, g_bpffunc_ctx, "bpffunc_config.ini");
}

PLUG_API IDFUNC_S * BPFFUNC_GetCtx(void)
{
    return g_bpffunc_ctx;
}
