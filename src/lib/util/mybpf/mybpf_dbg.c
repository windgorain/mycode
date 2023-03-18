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
    {"loader", "process", MYBPF_DBG_ID_LOADER, MYBPF_DBG_FLAG_LOADER_PROCESS},
    {"loader", "progs", MYBPF_DBG_ID_LOADER, MYBPF_DBG_FLAG_LOADER_PROGS},
    {"elf", "process", MYBPF_DBG_ID_ELF, MYBPF_DBG_FLAG_ELF_PROCESS},
    {"vm", "process", MYBPF_DBG_ID_VM, MYBPF_DBG_FLAG_VM_PROCESS},
    {"relo", "process", MYBPF_DBG_ID_RELO, MYBPF_DBG_FLAG_RELO_PROCESS},

    {0}
};

DBG_UTL_CTRL_S g_mybpf_runtime_dbg = DBG_INIT_VALUE("mybpf",
        g_mybpf_runtime_dbg_flags, g_mybpf_runtime_dbg_defs, MYBPF_DBG_ID_MAX);


