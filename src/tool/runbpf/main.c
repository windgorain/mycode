/*********************************************************
*   Copyright (C), Xingang.Li
*   Author:      Xingang.Li  Version: 1.0
*   Description: 
*
********************************************************/
#include "bs.h"
#include "utl/args_utl.h"
#include "utl/time_utl.h"
#include "utl/subcmd_utl.h"
#include "utl/getopt2_utl.h"
#include "utl/mybpf_loader.h"
#include "utl/mybpf_prog.h"
#include "utl/mybpf_vm.h"
#include "utl/mybpf_elf.h"
#include "utl/mybpf_relo.h"
#include "utl/mybpf_file.h"
#include "utl/mybpf_jit.h"
#include "utl/mybpf_simple.h"
#include "utl/mybpf_dbg.h"
#include "utl/mybpf_asm.h"
#include "utl/mybpf_asmexp.h"
#include "utl/mybpf_bare.h"
#include "utl/ubpf_utl.h"
#include "utl/file_utl.h"
#include "utl/mybpf_merge.h"
#include "utl/umap_def.h"

#define RUNBPF_ALL_CAPABILITY 1

static void _runbpf_opt_help(GETOPT2_NODE_S *opt)
{
    char buf[4096];
    printf("%s", GETOPT2_BuildHelpinfo(opt, buf, sizeof(buf)));
    return;
}

#if (RUNBPF_ALL_CAPABILITY)
static int _runbpf_run_file(char *file, char *sec_name, int jit, char *params)
{
    char *argv[32];
    int argc = 0;
    MYBPF_FILE_CTX_S ctx = {0};
    MYBPF_PARAM_S p = {0};

    if (! sec_name) {
        sec_name = "tcmd/";
    }

    if (params) {
        argc = ARGS_Split(params, argv, ARRAY_SIZE(argv));
    }

    ctx.file = file;
    ctx.sec_name = sec_name;
    ctx.jit = jit;

    p.p[0] = argc;
    p.p[1] = (long)argv;

    int ret = MYBPF_RunFileExt(&ctx, &p);
    if (ret < 0) {
        printf("Run file failed. \r\n");
        ErrCode_Print();
        return ret;
    }

    return ctx.bpf_ret;
}

static int _runbpf_file(int argc, char **argv)
{
    char *filename=NULL;
    char *params = NULL;
    char *sec_name= NULL;
    GETOPT2_NODE_S opt[] = {
        {'P', 0, "filename", GETOPT2_V_STRING, &filename, "bpf file name", 0},
        {'o', 's', "section", GETOPT2_V_STRING, &sec_name, "section prefix", 0},
        {'o', 'j', "jit", GETOPT2_V_NONE, NULL, "jit", 0},
        {'o', 'p', "params", GETOPT2_V_STRING, &params, "params", 0},
        {'o', 'd', "debug", GETOPT2_V_NONE, NULL, "print debug info", 0},
        {0} };

    int jit = 0;

    if (BS_OK != GETOPT2_Parse(argc, argv, opt)) {
        _runbpf_opt_help(opt);
        return -1;
    }

    if (filename == NULL) {
        _runbpf_opt_help(opt);
        return -1;
    }

    if (GETOPT2_IsOptSetted(opt, 'j', NULL)) {
        jit = 1;
    }

    if (GETOPT2_IsOptSetted(opt, 'd', NULL)) {
        MYBPF_DBG_SetAllDebugFlags();
    }

    return _runbpf_run_file(filename, sec_name, jit, params);
}

static int _runbpf_bare(int argc, char **argv)
{
    char *filename=NULL;
    char *params = NULL;
    GETOPT2_NODE_S opt[] = {
        {'P', 0, "filename", GETOPT2_V_STRING, &filename, "bpf file name", 0},
        {'o', 'p', "params", GETOPT2_V_STRING, &params, "params", 0},
        {'o', 'd', "debug", GETOPT2_V_NONE, NULL, "print debug info", 0},
        {0} };

    if (BS_OK != GETOPT2_Parse(argc, argv, opt)) {
        _runbpf_opt_help(opt);
        return -1;
    }

    if (filename == NULL) {
        _runbpf_opt_help(opt);
        return -1;
    }

    if (GETOPT2_IsOptSetted(opt, 'd', NULL)) {
        MYBPF_DBG_SetAllDebugFlags();
    }

    return MYBPF_RunBareFile(filename, NULL, params);
}

static void _walk_prog_dump_mem(void *data, int len)
{
    int size = len;
    int print_size;
    char *d = data;

    while (size > 0) {
        print_size = MIN(8, size);
        MEM_PrintCFormat(d, print_size, NULL);
        d += print_size;
        size -= print_size;
    }
}

static int _walk_prog_dump(void *data, ELF_PROG_INFO_S *info, void *ud)
{
    USER_HANDLE_S *uh = ud;
    char *func = uh->ahUserHandle[0];
    UINT flag = HANDLE_UINT(uh->ahUserHandle[1]);

    if ((func) && (strcmp(func, info->func_name) != 0)) {
        return 0;
    }

    printf("sec:%s, name:%s, offset:%d \n", info->sec_name, info->func_name, info->prog_offset);

    if (flag == 0) {
        _walk_prog_dump_mem(data, info->size);
    } else {
        MYBPF_ASM_DumpAsm(data, info->size, flag);
    }

    printf("\n");

    return 0;
}

static int _dump_spf_prog(char *filename, void *ud)
{
    void *m = MYBPF_SIMPLE_OpenFile(filename);
    if (! m) {
        printf("Can'open process file %s \n", filename);
        return -1;
    }

    MYBPF_SIMPLE_WalkProg(m, _walk_prog_dump, ud);

    MYBPF_SIMPLE_Close(m);

    return 0;
}

static int _dump_prog(int argc, char **argv)
{
    char *filename=NULL;
    char *function = NULL;
    GETOPT2_NODE_S opt[] = {
        {'o', 'f', "function", GETOPT2_V_STRING, &function, "function name", 0},
        {'o', 'd', "disassemble", GETOPT2_V_NONE, NULL, "show disassemble", 0},
        {'o', 'e', "expression", GETOPT2_V_NONE, NULL, "show expression", 0},
        {'o', 'r', "raw", GETOPT2_V_NONE, NULL, "show raw data", 0},
        {'o', 'l', "line", GETOPT2_V_NONE, NULL, "show line number", 0},
        {'P', 0, "filename", GETOPT2_V_STRING, &filename, "bpf file name", 0},
        {0} };
    USER_HANDLE_S uh;
    UINT flag = 0;

    if (BS_OK != GETOPT2_Parse(argc, argv, opt)) {
        _runbpf_opt_help(opt);
        return -1;
    }

    if (GETOPT2_IsOptSetted(opt, 'l', NULL)) {
        flag |= MYBPF_DUMP_FLAG_LINE;
    }

    if (GETOPT2_IsOptSetted(opt, 'd', NULL)) {
        flag |= MYBPF_DUMP_FLAG_ASM;
    }

    if (GETOPT2_IsOptSetted(opt, 'e', NULL)) {
        flag |= MYBPF_DUMP_FLAG_EXP;
    }

    if (GETOPT2_IsOptSetted(opt, 'r', NULL)) {
        flag |= MYBPF_DUMP_FLAG_RAW;
    }

    uh.ahUserHandle[0] = function;
    uh.ahUserHandle[1] = UINT_HANDLE(flag);

    return _dump_spf_prog(filename, &uh);
}

static int _show_simple_file(char *filename)
{
    char info[4096] = "";

    int ret = MYBPF_SIMPLE_BuildFileInfo(filename, info, sizeof(info));
    if (ret < 0) {
        ErrCode_Print();
    }

    printf("%s", info);

    return 0;
}

static int _show_global_info(int argc, char **argv)
{
    char *filename=NULL;
    GETOPT2_NODE_S opt[] = {
        {'P', 0, "filename", GETOPT2_V_STRING, &filename, "bpf file name", 0},
        {'o', 'd', "debug", GETOPT2_V_NONE, NULL, "print debug info", 0},
        {0} };

    if (BS_OK != GETOPT2_Parse(argc, argv, opt)) {
        _runbpf_opt_help(opt);
        return -1;
    }

    if (GETOPT2_IsOptSetted(opt, 'd', NULL)) {
        MYBPF_DBG_SetAllDebugFlags();
    }

    return MYBPF_ShowPcAccessGlobal(filename);
}

static int _show_file(int argc, char **argv)
{
    char *filename=NULL;
    GETOPT2_NODE_S opt[] = {
        {'P', 0, "filename", GETOPT2_V_STRING, &filename, "bpf file name", 0},
        {'o', 'd', "debug", GETOPT2_V_NONE, NULL, "print debug info", 0},
        {0} };

    if (BS_OK != GETOPT2_Parse(argc, argv, opt)) {
        _runbpf_opt_help(opt);
        return -1;
    }

    if (GETOPT2_IsOptSetted(opt, 'd', NULL)) {
        MYBPF_DBG_SetAllDebugFlags();
    }

    return _show_simple_file(filename);
}

static int _walk_prog_export(void *data, ELF_PROG_INFO_S *info, void *ud)
{
    char *func = ud;
    FILE *fp;
    char buf[256];
    char *sec_name = info->sec_name;
    char *func_name = info->func_name;

    if ((func) && (strcmp(func, func_name) != 0)) {
        return 0;
    }

    if (TXT_IS_EMPTY(func_name)) {
        if (TXT_IS_EMPTY(sec_name)) {
            snprintf(buf, sizeof(buf), "noname_%u.bpf", info->prog_offset);
        } else {
            while (*sec_name == '.') {
                sec_name ++;
            }
            snprintf(buf, sizeof(buf), "%s_%u.bpf", sec_name, info->prog_offset);
            TXT_ReplaceChar(buf, '/', '_');
        }
    } else {
        snprintf(buf, sizeof(buf), "%s.bpf", func_name);
    }

    fp = FILE_Open(buf, TRUE, "wb+");
    if (! fp) {
        fprintf(stderr, "Can't open %s \r\n", buf);
        return 0;
    }

    fwrite(data, 1, info->size, fp);

    FILE_Close(fp);

    return 0;
}

static int _export_spf_prog(char *filename, char *function)
{
    void *m = MYBPF_SIMPLE_OpenFile(filename);
    if (! m) {
        printf("Can'open process file %s \n", filename);
        return -1;
    }

    MYBPF_SIMPLE_WalkProg(m, _walk_prog_export, function);

    MYBPF_SIMPLE_Close(m);

    return 0;
}

static int _export_prog(int argc, char **argv)
{
    char *filename=NULL;
    char *function = NULL;
    GETOPT2_NODE_S opt[] = {
        {'o', 'f', "function", GETOPT2_V_STRING, &function, "function name", 0},
        {'P', 0, "filename", GETOPT2_V_STRING, &filename, "bpf file name", 0},
        {0} };

    if (BS_OK != GETOPT2_Parse(argc, argv, opt)) {
        _runbpf_opt_help(opt);
        return -1;
    }

    return _export_spf_prog(filename, function);
}


static int _convert_bpfexp(int argc, char **argv)
{
    char *filename=NULL;
    char *output_name=NULL;
    int binary = 0;
    GETOPT2_NODE_S opt[] = {
        {'P', 0, "filename", GETOPT2_V_STRING, &filename, "bpf file name", 0},
        {'o', 'o', "output-name", GETOPT2_V_STRING, &output_name, "output name", 0},
        {'o', 'd', "debug", GETOPT2_V_NONE, NULL, "print debug info", 0},
        {'o', 'b', "binary", GETOPT2_V_NONE, NULL, "binary format", 0},
        {0} };

    char bpfexp_file[256];

    if (0 != GETOPT2_Parse(argc, argv, opt)) {
        _runbpf_opt_help(opt);
        return -1;
    }

    if (GETOPT2_IsOptSetted(opt, 'd', NULL)) {
        MYBPF_DBG_SetAllDebugFlags();
    }

    if (GETOPT2_IsOptSetted(opt, 'b', NULL)) {
        binary = 1;
    }

    if (output_name) {
        strlcpy(bpfexp_file, output_name, sizeof(bpfexp_file));
    } else {
        snprintf(bpfexp_file, sizeof(bpfexp_file), "%s.bpfexp.c", filename);
    }

    int ret = MYBPF_ASMEXP_ConvertFile(filename, bpfexp_file, binary);
    if (ret < 0) {
        ErrCode_Print();
    }

    return ret;
}

static int _convert_merge(int argc, char **argv)
{
    char *file1 =NULL;
    char *file2 =NULL;
    char *output_name = "output.spf";
    GETOPT2_NODE_S opt[] = {
        {'P', 0, "file1", GETOPT2_V_STRING, &file1, "file1", 0},
        {'P', 0, "file2", GETOPT2_V_STRING, &file2, "file2", 0},
        {'o', 'o', "output-name", GETOPT2_V_STRING, &output_name, "output name", 0},
        {0} };

    if (0 != GETOPT2_Parse(argc, argv, opt)) {
        _runbpf_opt_help(opt);
        return -1;
    }

    return MYBPF_Merge(file1, file2, output_name);
}

static int _runbpf_set_map(int argc, char **argv)
{
    char *file =NULL;
    char *output_name = NULL;
    char *map_name = NULL;
    FILE_MEM_S *m;
    UMAP_ELF_MAP_S map = {0};
    MYBPF_SIMPLE_MAP_MASK_S msk = {0};
    int id = -1;
    int ret;
    GETOPT2_NODE_S opt[] = {
        {'P', 0, "file", GETOPT2_V_STRING, &file, "file name", 0},
        {'o', 'o', "output-name", GETOPT2_V_STRING, &output_name, "output name", 0},
        {'o', 'n', "name", GETOPT2_V_STRING, &map_name, "map name", 0},
        {'o', 'i', "id", GETOPT2_V_U32, &id, "map id", 0},
        {'o', 't', "type", GETOPT2_V_U32, &map.type, "map type", 0},
        {'o', 'k', "key", GETOPT2_V_U32, &map.size_key, "map key size", 0},
        {'o', 'v', "value", GETOPT2_V_U32, &map.size_value, "map value size", 0},
        {'o', 'm', "max-elem", GETOPT2_V_U32, &map.max_elem, "map max elem", 0},
        {'o', 'f', "flags", GETOPT2_V_U32, &map.flags, "map flags", 0},
        {0} };
    char simple_file[256];

    if (0 != GETOPT2_Parse(argc, argv, opt)) {
        _runbpf_opt_help(opt);
        return -1;
    }

    if (output_name) {
        strlcpy(simple_file, output_name, sizeof(simple_file));
    } else {
        snprintf(simple_file, sizeof(simple_file), "%s.spf", file);
    }

    if (GETOPT2_IsOptSetted(opt, 't', NULL)) {
        msk.type = 1;
    }

    if (GETOPT2_IsOptSetted(opt, 'k', NULL)) {
        msk.size_key = 1;
    }

    if (GETOPT2_IsOptSetted(opt, 'v', NULL)) {
        msk.size_value = 1;
    }

    if (GETOPT2_IsOptSetted(opt, 'm', NULL)) {
        msk.max_elem = 1;
    }

    if (GETOPT2_IsOptSetted(opt, 'f', NULL)) {
        msk.flags = 1;
    }

    m = MYBPF_SIMPLE_OpenFile(file);
    if (! m) {
        printf("Can't open file %s \n", file);
        return -1;
    }

    if (map_name) {
        id = MYBPF_SIMPLE_GetMapIDByName(m, map_name);
    }

    if (id < 0) {
        printf("Invalid map name or map id \n");
        ret = id;
        goto _OUT;
    }

    ret = MYBPF_SIMPLE_ModifyMap(m, id, &map, &msk);
    if (ret < 0) {
        goto _OUT;
    }

    ret = MYBPF_SIMPLE_WriteFile(m, simple_file);

_OUT:
    if (m) {
        MYBPF_SIMPLE_Close(m);
    }

    return ret;
}

#endif

static MYBPF_SIMPLE_CONVERT_CALL_MAP_S * _build_convert_calls_map(char *file)
{
    char *line;
    char buf[64];

    FILE *fp = fopen(file, "r");
    if (! fp) {
        printf("Can't open file %s \n", file);
        return NULL;
    }

    int count = 0;
    while ((line = fgets(buf, sizeof(buf), fp)) != NULL) {
        count ++;
    }

    MYBPF_SIMPLE_CONVERT_CALL_MAP_S * map = MEM_ZMalloc((count + 1) * sizeof(MYBPF_SIMPLE_CONVERT_CALL_MAP_S));
    if (! map) {
        printf("Can't malloc memory \n");
        fclose(fp);
        return NULL;
    }

    int index = 0;
    int ele_index = 0;
    int base_offset = 0;
    int ele_size = 0;

    fseek(fp, 0, SEEK_SET);
    while ((line = fgets(buf, sizeof(buf), fp)) != NULL) {
        char *split = strchr(buf, ':');
        if (split ) {
            *split = '\0';
        }

        if (strcmp(buf, "ele_size") == 0) {
            ele_size = strtol(split + 1, NULL, 10);
            continue;
        }

        if (strcmp(buf, "offset") == 0) {
            ele_index = 0; 
            base_offset = strtol(split + 1, NULL, 16);
            continue;
        }

        int imm = strtol(buf, NULL, 10);
        if (imm) {
            
            map[index].imm = imm;
            map[index].new_imm = base_offset + ele_index * ele_size;
            index ++;
        }

        ele_index ++;
    }

    
    map[index].imm = 0;

    fclose(fp);

    return map;
}

static int _porcess_mode(int mode, char *raw_map_file, OUT MYBPF_SIMPLE_CONVERT_PARAM_S *p)
{
    if (mode == 1) {
        p->helper_mode = MYBPF_JIT_HELPER_MODE_ARRAY;
        p->aot_map_index_to_ptr = 1;
    } else if (mode == 2) {
        if (! raw_map_file) {
            PRINTLN_HYELLOW("Need --mapfile");
            return -1;
        }
        if (! (p->helper_map = _build_convert_calls_map(raw_map_file))) {
            return -1;
        }
        p->helper_mode = MYBPF_JIT_HELPER_MODE_RAW;
    } else if (mode == 3) {
        p->helper_mode = MYBPF_JIT_HELPER_MODE_AGENT;
        p->param_6th = 1;
    } else if (mode == 4) {
        p->helper_mode = MYBPF_JIT_HELPER_MODE_ARRAY;
        p->aot_map_index_to_ptr = 1;
        p->param_6th = 1;
    } else {
        p->helper_mode = MYBPF_JIT_HELPER_MODE_AGENT;
    }

    return 0;
}

static int _init_convert_param(OUT MYBPF_SIMPLE_CONVERT_PARAM_S *p,
        GETOPT2_NODE_S *opt, char *jit_arch, int mode, char *map_file)
{
    if (GETOPT2_IsOptSetted(opt, 'j', NULL)) {
        if (jit_arch) {
            p->jit_arch = MYBPF_JIT_GetJitTypeByName(jit_arch);
        } else {
            p->jit_arch = MYBPF_JIT_LocalArch();
        }

        if (! p->jit_arch) {
            PRINTLN_HYELLOW("Can't support AOT, to use raw code");
        }
    }

    if (p->jit_arch) {
        p->translate_mode_aot = 1;
    }

    if (GETOPT2_IsOptSetted(opt, 0, "with-map-name")) {
        p->with_map_name = 1;
    }

    if (GETOPT2_IsOptSetted(opt, 0, "with-func-name")) {
        p->with_func_name = 1;
    }

    if (_porcess_mode(mode, map_file, p) < 0) {
        return -1;
    }

    return 0;
}

static void _build_simple_file_name(char *from, char *to, char *ext, OUT char *dst, int size)
{
    if (to) {
        strlcpy(dst, to, size);
    } else {
        snprintf(dst, size, "%s.%s", from, ext);
    }
}

static int _convert_simple(int argc, char **argv)
{
    char *filename=NULL;
    char *output_name=NULL;
    char *jit_arch_name =NULL;
    char *convert_map_file=NULL;
    U32 mode = 0;
    int ret;
    GETOPT2_NODE_S opt[] = {
        {'P', 0, "filename", GETOPT2_V_STRING, &filename, "bpf file name", 0},
        {'o', 'j', "jit", GETOPT2_V_NONE, NULL, "jit/aot", 0},
        {'o', 't', "target", GETOPT2_V_STRING, &jit_arch_name, "select target arch: arm64,x86_64", 0},
        {'o', 'm', "mode", GETOPT2_V_U32, &mode, "for aot mode,0:agent(default),1:array,2:raw,3:ragent,4:rarray", 0},
        {'o', 'f', "mapfile", GETOPT2_V_STRING, &convert_map_file, "for aot raw mode", 0},
        {'o', 'o', "output-name", GETOPT2_V_STRING, &output_name, "output name", 0},
        {'o', 'd', "debug", GETOPT2_V_NONE, NULL, "print debug info", 0},
        {'o', 0, "with-map-name", GETOPT2_V_NONE, NULL, "with map name ", 0},
        {'o', 0, "with-func-name", GETOPT2_V_NONE, NULL, "with function name ", 0},
        {0} };
    char simple_file[256];
    MYBPF_SIMPLE_CONVERT_PARAM_S p = {0};

    if (0 != GETOPT2_Parse(argc, argv, opt)) {
        _runbpf_opt_help(opt);
        return -1;
    }

    if (GETOPT2_IsOptSetted(opt, 'd', NULL)) {
        MYBPF_DBG_SetAllDebugFlags();
    }

    _build_simple_file_name(filename, output_name, "spf", simple_file, sizeof(simple_file));

    ret = _init_convert_param(&p, opt, jit_arch_name, mode, convert_map_file);
    if (ret < 0) {
        return ret;
    }

    ret = MYBPF_SIMPLE_Convert2File(filename, simple_file, &p);
    if (ret < 0) {
        ErrCode_Print();
    }

    MEM_SafeFree(p.helper_map);

    return ret;
}

static int _convert_bare(int argc, char **argv)
{
    char *filename=NULL;
    char *output_name=NULL;
    char *jit_arch_name =NULL;
    char *convert_map_file=NULL;
    U32 mode = 0;
    int ret;
    GETOPT2_NODE_S opt[] = {
        {'P', 0, "filename", GETOPT2_V_STRING, &filename, "bpf file name", 0},
        {'o', 'j', "jit", GETOPT2_V_NONE, NULL, "jit/aot", 0},
        {'o', 't', "target", GETOPT2_V_STRING, &jit_arch_name, "select target arch: arm64,x86_64", 0},
        {'o', 'm', "mode", GETOPT2_V_U32, &mode, "for aot mode,0:agent(default),1:array,2:raw,3:ragent,4:rarray", 0},
        {'o', 'f', "mapfile", GETOPT2_V_STRING, &convert_map_file, "for aot raw mode", 0},
        {'o', 'o', "output-name", GETOPT2_V_STRING, &output_name, "output name", 0},
        {'o', 'd', "debug", GETOPT2_V_NONE, NULL, "print debug info", 0},
        {0} };
    char bare_file[256];
    MYBPF_SIMPLE_CONVERT_PARAM_S p = {0};

    if (0 != GETOPT2_Parse(argc, argv, opt)) {
        _runbpf_opt_help(opt);
        return -1;
    }

    if (GETOPT2_IsOptSetted(opt, 'd', NULL)) {
        MYBPF_DBG_SetAllDebugFlags();
    }

    _build_simple_file_name(filename, output_name, "bare", bare_file, sizeof(bare_file));

    ret = _init_convert_param(&p, opt, jit_arch_name, mode, convert_map_file);
    if (ret < 0) {
        return ret;
    }

    ret = MYBPF_BARE_Convert2File(filename, bare_file, &p);
    if (ret < 0) {
        ErrCode_Print();
    }

    MEM_SafeFree(p.helper_map);

    return ret;
}

int main(int argc, char **argv)
{
    static SUB_CMD_NODE_S subcmds[] = 
    {
#if (RUNBPF_ALL_CAPABILITY)
        {"run file", _runbpf_file, "run file"},
        {"run bare", _runbpf_bare, "run bare file"},
        {"show file", _show_file, "show file info"},
        {"show global", _show_global_info, "show global data info"},
        {"dump prog", _dump_prog, "dump prog"},
        {"export prog", _export_prog, "export prog"},
        {"convert asmexp", _convert_bpfexp, "convert to bpf expression file"},
#endif
        {"convert simple", _convert_simple, "convert to simple bpf file"},
#if (RUNBPF_ALL_CAPABILITY)
        {"convert bare", _convert_bare, "convert to bare file"},
        {"merge", _convert_merge, "merge spf files"},
        {"set map", _runbpf_set_map, "set map"},
#endif
        {NULL}
    };

    return SUBCMD_Do(subcmds, argc, argv);
}


