/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
********************************************************/
#include "bs.h"
#include "utl/mybpf_vm.h"
#include "utl/bpf_helper_utl.h"
#include "utl/mybpf_runtime.h"
#include "utl/mybpf_loader.h"
#include "utl/mybpf_prog.h"
#include "utl/mybpf_elf.h"
#include "utl/mybpf_file.h"

static void _mybpf_file_walk_prog(ELF_S *elf, PF_MYBPF_FILE_WALK_PROG walk_func, void *ud)
{
    void *iter = NULL;
    ELF_SECTION_S sec;
    int ret;
    char *func_name;

    while ((iter = ELF_GetNextSection(elf, iter, &sec))) {
        if (! ELF_IsProgSection(&sec)) {
            continue;
        }

        func_name = ELF_GetSecSymbolName(elf, sec.sec_id, STT_FUNC, 0);
        if (! func_name) {
            continue;
        }

        ret = walk_func(sec.data->d_buf, sec.data->d_size, sec.shname, func_name, ud);
        if (ret != BS_WALK_CONTINUE) {
            break;
        }
    }
}

static int _mybpf_file_walk_map(ELF_S *elf, PF_MYBPF_FILE_WALK_MAP walk_func, void *ud)
{
    char *map;
    char *map_name;
    int i;
    int ret;
    MYBPF_MAPS_SEC_S map_sec;

    ret = MYBPF_ELF_GetMapsSection(elf, &map_sec);
    if (ret < 0) {
        return ret;
    }

    if (map_sec.map_count == 0) {
        return 0;
    }

    map = map_sec.maps;

    for (i=0; i<map_sec.map_count; i++) {
        map_name = ELF_GetSecSymbolName(elf, map_sec.sec_id, 0, i);
        ret = walk_func(map, map_sec.map_def_size, map_name, ud);
        if (ret != BS_WALK_CONTINUE) {
            break;
        }
        map = map + map_sec.map_def_size;
    }

    return 0;
}

int MYBPF_WalkProg(char *file, PF_MYBPF_FILE_WALK_PROG walk_func, void *ud)
{
    ELF_S elf;

    int ret = ELF_Open(file, &elf);
    if (ret < 0) {
        RETURNI(BS_CAN_NOT_OPEN, "Can't open file %s \r\n", file);
    }

    _mybpf_file_walk_prog(&elf, walk_func, ud);

    ELF_Close(&elf);

    return 0;
}

int MYBPF_WalkMap(char *file, PF_MYBPF_FILE_WALK_MAP walk_func, void *ud)
{
    ELF_S elf;

    int ret = ELF_Open(file, &elf);
    if (ret < 0) {
        RETURNI(BS_CAN_NOT_OPEN, "Can't open file %s \r\n", file);
    }

    _mybpf_file_walk_map(&elf, walk_func, ud);

    ELF_Close(&elf);

    return 0;
}

int MYBPF_RunFile(char *file, char *func, OUT UINT64 *bpf_ret,
        UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5)
{
    int ret;
    MYBPF_CTX_S ctx = {0};
    MYBPF_LOADER_PARAM_S p = {0};
    MYBPF_RUNTIME_S runtime;

    if ((ret = MYBPF_RuntimeInit(&runtime, 128)) < 0) {
        return ret;
    }

    p.instance = "runfile";
    p.filename = file;
    p.func_name = func;

    if ((ret = MYBPF_LoaderLoad(&runtime, &p)) < 0) {
        return ret;
    }

    int fd = MYBPF_PROG_GetByFuncName(&runtime, "runfile", func);
    if (fd < 0) {
        return fd;
    }

    ctx.insts = MYBPF_PROG_RefByFD(&runtime, fd);

    ret = MYBPF_DefultRun(&ctx, p1, p2, p3, p4, p5);
    if (ret < 0) {
        return ret;
    }

    MYBPF_RuntimeFini(&runtime);

    if (bpf_ret) {
        *bpf_ret = ctx.bpf_ret;
    }

    return 0;
}

