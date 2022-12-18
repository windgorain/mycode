/*****************************************************************************
*   Created by LiXingang, Copyright LiXingang
*   Description: 
*
*****************************************************************************/
#include "bs.h"
#include "utl/idfunc_utl.h"
#include "utl/mybpf_runtime.h"
#include "utl/mybpf_loader.h"
#include "utl/mybpf_prog.h"
#include "utl/cff_utl.h"

static void _idfunc_walk_config(HANDLE cff, char *tag, HANDLE ud)
{
    USER_HANDLE_S *uh = ud;
    MYBPF_RUNTIME_S *rt = uh->ahUserHandle[0];
    IDFUNC_S *ctrl = uh->ahUserHandle[1];
    char *file, *sec_name;
    MYBPF_PROG_NODE_S *prog;
    UINT id;
    MYBPF_LOADER_PARAM_S p = {0};

    id = TXT_Str2Ui(tag);

    if (CFF_GetPropAsString(cff, tag, "file", &file) < 0) {
        return;
    }
    if (CFF_GetPropAsString(cff, tag, "sec_name", &sec_name) < 0) {
        return;
    }

    p.filename = file;
    p.sec_name = sec_name;
    p.instance = tag;

    if (MYBPF_LoaderLoad(rt, &p) < 0) {
        return;
    }

    int fd = MYBPF_PROG_GetBySecName(rt, tag, sec_name);
    if (fd < 0) {
        return;
    }

    prog = MYBPF_PROG_RefByFD(rt, fd);
    BS_DBGASSERT(prog);

    IDFUNC_SetNode(ctrl, id, IDFUNC_TYPE_BPF, prog->insn);
}

int IDFUNC_Load(MYBPF_RUNTIME_S *rt, IDFUNC_S *ctrl, char *conf_file)
{
    CFF_HANDLE cff;
    USER_HANDLE_S uh;

    cff = CFF_INI_Open(conf_file, CFF_FLAG_READ_ONLY);
    if (! cff) {
        RETURN(BS_CAN_NOT_OPEN);
    }

    uh.ahUserHandle[0] = rt;
    uh.ahUserHandle[1] = ctrl;

    CFF_WalkTag(cff, _idfunc_walk_config, &uh);

    CFF_Close(cff);

    return 0;
}

