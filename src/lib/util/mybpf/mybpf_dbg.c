/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
********************************************************/
#include "bs.h"
#include "utl/dbg_utl.h"
#include "utl/mybpf_dbg.h"

static UINT g_mybpf_runtime_dbg_flags[MYBPF_DBG_ID_MAX] = {0};

static DBG_UTL_DEF_S g_mybpf_runtime_dbg_defs[] = 
{
    {"loader", "process", MYBPF_DBG_ID_LOADER, DBG_UTL_FLAG_PROCESS},
    {"loader", "progs", MYBPF_DBG_ID_LOADER, MYBPF_DBG_FLAG_LOADER_PROGS},
    {"elf", "process", MYBPF_DBG_ID_ELF, DBG_UTL_FLAG_PROCESS},
    {"vm", "process", MYBPF_DBG_ID_VM, DBG_UTL_FLAG_PROCESS},
    {"relo", "process", MYBPF_DBG_ID_RELO, DBG_UTL_FLAG_PROCESS},
    {"jit", "process", MYBPF_DBG_ID_JIT, DBG_UTL_FLAG_PROCESS},

    {0}
};

DBG_UTL_CTRL_S g_mybpf_runtime_dbg = DBG_INIT_VALUE("mybpf",
        g_mybpf_runtime_dbg_flags, g_mybpf_runtime_dbg_defs, MYBPF_DBG_ID_MAX);


