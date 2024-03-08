/*****************************************************************************
*   Created by LiXingang, Copyright LiXingang
*   Description: 
*
*****************************************************************************/
#include "bs.h"
#include "utl/bfunc_utl.h"
#include "utl/mybpf_runtime.h"
#include "utl/mybpf_loader.h"
#include "utl/mybpf_prog.h"
#include "utl/cff_utl.h"

static void _bfunc_walk_config(HANDLE cff, char *tag, HANDLE ud)
{
    BFUNC_S *ctrl = ud;
    MYBPF_RUNTIME_S *rt = ctrl->runtime;
    char *file, *sec_name = NULL, *func_name = NULL;
    MYBPF_PROG_NODE_S *prog;
    UINT id = (UINT)(int)-1;
    MYBPF_LOADER_PARAM_S p = {0};

    if (CFF_GetPropAsString(cff, tag, "file", &file) < 0) {
        return;
    }

    CFF_GetPropAsString(cff, tag, "sec_name", &sec_name);
    CFF_GetPropAsString(cff, tag, "func_name", &func_name);
    CFF_GetPropAsUint(cff, tag, "id", &id);

    if (id == (UINT)(int)-1) {
        return;
    }

    
    if ((! sec_name) && (! func_name)) {
        return;
    }

    p.instance = tag;
    p.filename = file;

    if (MYBPF_LoaderLoad(rt, &p) < 0) {
        return;
    }

    if (sec_name) {
        prog = MYBPF_PROG_GetBySecName(rt, tag, sec_name);
    } else  {
        prog = MYBPF_PROG_GetByFuncName(rt, tag, func_name);
    }

    if (! prog) {
        return;
    }

    MYBPF_LOADER_NODE_S *n = prog->loader_node;

    BFUNC_Set(ctrl, id, n->jitted, prog, prog->insn, n->insts, (char*)n->insts + n->insts_len);
}

int BFUNC_Load(BFUNC_S *ctrl, char *conf_file)
{
    CFF_HANDLE cff;

    cff = CFF_INI_Open(conf_file, CFF_FLAG_READ_ONLY);
    if (! cff) {
        RETURN(BS_CAN_NOT_OPEN);
    }

    CFF_WalkTag(cff, _bfunc_walk_config, ctrl);

    CFF_Close(cff);

    return 0;
}

