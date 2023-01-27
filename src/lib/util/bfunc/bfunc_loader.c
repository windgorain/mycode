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
    USER_HANDLE_S *uh = ud;
    MYBPF_RUNTIME_S *rt = uh->ahUserHandle[0];
    BFUNC_S *ctrl = uh->ahUserHandle[1];
    char *file, *sec_name = NULL, *func_name = NULL;
    MYBPF_PROG_NODE_S *prog;
    UINT id = (UINT)(int)-1;
    int fd;
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

    /* sec name 和 func name至少要指定一个 */
    if ((! sec_name) && (! func_name)) {
        return;
    }

    p.instance = tag;
    p.filename = file;
    p.sec_name = sec_name;
    p.func_name = func_name;

    if (MYBPF_LoaderLoad(rt, &p) < 0) {
        return;
    }

    if (sec_name) {
        fd = MYBPF_PROG_GetBySecName(rt, tag, sec_name);
    } else  {
        fd = MYBPF_PROG_GetByFuncName(rt, tag, func_name);
    }

    if (fd < 0) {
        return;
    }

    prog = MYBPF_PROG_RefByFD(rt, fd);
    BS_DBGASSERT(prog);

    BFUNC_Set(ctrl, id, 0, fd, prog->insn);
}

int BFUNC_Load(MYBPF_RUNTIME_S *rt, BFUNC_S *ctrl, char *conf_file)
{
    CFF_HANDLE cff;
    USER_HANDLE_S uh;

    cff = CFF_INI_Open(conf_file, CFF_FLAG_READ_ONLY);
    if (! cff) {
        RETURN(BS_CAN_NOT_OPEN);
    }

    uh.ahUserHandle[0] = rt;
    uh.ahUserHandle[1] = ctrl;

    CFF_WalkTag(cff, _bfunc_walk_config, &uh);

    CFF_Close(cff);

    return 0;
}

