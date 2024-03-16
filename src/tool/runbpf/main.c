/*********************************************************
*   Copyright (C), Xingang.Li
*   Author:      Xingang.Li  Version: 1.0
*   Description: 
*
********************************************************/
#include "bs.h"
#include "utl/args_utl.h"
#include "utl/subcmd_utl.h"
#include "utl/getopt2_utl.h"
#include "utl/arch_utl.h"
#include "utl/mybpf_loader.h"
#include "utl/mybpf_prog.h"
#include "utl/mybpf_file.h"
#include "utl/mybpf_jit.h"
#include "utl/mybpf_simple.h"
#include "utl/mybpf_dbg.h"
#include "utl/mybpf_asm.h"
#include "utl/mybpf_bare.h"
#include "utl/mybpf_bare_convert.h"

static void _runbpf_opt_help(GETOPT2_NODE_S *opt)
{
    char buf[4096];
    printf("%s", GETOPT2_BuildHelpinfo(opt, buf, sizeof(buf)));
    return;
}

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

static int _runbpf_run_bare(char *filename, char *params)
{
    char *argv[32];
    int argc = 0;
    MYBPF_PARAM_S p = {0};

    if (params) {
        argc = ARGS_Split(params, argv, ARRAY_SIZE(argv));
    }

    p.p[0] = argc;
    p.p[1] = (long)argv;
 
    return MYBPF_RunBareFile(filename, NULL, &p);
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

    return _runbpf_run_bare(filename, params);
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
    FILE_MEM_S m;
    int ret;

    ret = MYBPF_SIMPLE_OpenFile(filename, &m);
    if (ret < 0) {
        printf("Can'open process file %s \n", filename);
        return -1;
    }

    MYBPF_SIMPLE_WalkProg(&m, _walk_prog_dump, ud);

    MYBPF_SIMPLE_Close(&m);

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
    p->aot_mode = mode;

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
            p->jit_arch = ARCH_GetTypeByName(jit_arch);
        } else {
            p->jit_arch = ARCH_LocalArch();
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
    int ret;
    U32 app_ver = 0;
    GETOPT2_NODE_S opt[] = {
        {'P', 0, "filename", GETOPT2_V_STRING, &filename, "bpf file name", 0},
        {'o', 't', "target", GETOPT2_V_STRING, &jit_arch_name, "select target arch: arm64,x86_64", 0},
        {'o', 'o', "output-name", GETOPT2_V_STRING, &output_name, "output name", 0},
        {'o', 'd', "debug", GETOPT2_V_NONE, NULL, "print debug info", 0},
        {'o', 'v', "version", GETOPT2_V_U32, &app_ver, "app version", 0},
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

    if (jit_arch_name) {
        p.jit_arch = ARCH_GetTypeByName(jit_arch_name);
    } else {
        p.jit_arch = ARCH_LocalArch();
    }

    if (! p.jit_arch) {
        PRINTLN_HYELLOW("Can't AOT to this target");
        return -1;
    }

    
    p.translate_mode_aot = 1;

    
    p.helper_mode = MYBPF_JIT_HELPER_MODE_ARRAY;
    p.aot_map_index_to_ptr = 1;
    p.param_6th = 1;

    p.app_ver = app_ver;

    ret = MYBPF_BARE_Convert2File(filename, bare_file, &p);
    if (ret < 0) {
        ErrCode_Print();
    }

    return ret;
}

int main(int argc, char **argv)
{
    static SUB_CMD_NODE_S subcmds[] = 
    {
        {"run file", _runbpf_file, "run file"},
        {"run bare", _runbpf_bare, "run bare file"},
        {"show file", _show_file, "show file info"},
        {"dump prog", _dump_prog, "dump prog"},
        {"convert simple", _convert_simple, "convert to simple bpf file"},
        {"convert bare", _convert_bare, "convert to bare file"},
        {NULL}
    };

    return SUBCMD_Do(subcmds, argc, argv);
}


