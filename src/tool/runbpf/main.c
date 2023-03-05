/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/args_utl.h"
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
#include "utl/ubpf_utl.h"
#include "utl/file_utl.h"

#define RUNBPF_ALL_CAPABILITY 1

#if (RUNBPF_ALL_CAPABILITY)
static int _runbpf_file(int argc, char **argv);
static int _dump_prog(int argc, char **argv);
static int _show_simple(int argc, char **argv);
static int _show_prog(int argc, char **argv);
static int _export_prog(int argc, char **argv);
#endif

static int _convert_simple(int argc, char **argv);

static SUB_CMD_NODE_S g_subcmds[] = 
{
#if (RUNBPF_ALL_CAPABILITY)
    {"run", _runbpf_file, "run file"},
    {"dump prog", _dump_prog, "dump prog"},
    {"show simple", _show_simple, "show simple bpf file"},
    {"show prog", _show_prog, "show prog"},
    {"export prog", _export_prog, "export prog"},
#endif
    {"convert simple", _convert_simple, "convert to simple bpf file"},
    {NULL, NULL}
};

static void _runbpf_opt_help(GETOPT2_NODE_S *opt)
{
    char buf[512];
    printf("%s", GETOPT2_BuildHelpinfo(opt, buf, sizeof(buf)));
    return;
}

#if (RUNBPF_ALL_CAPABILITY)
static int _runbpf_run_file(char *file, char *sec_prefix, int jit, char *params)
{
    char *argv[32];
    int argc = 0;
    MYBPF_FILE_CTX_S ctx = {0};

    if (! sec_prefix) {
        sec_prefix = "tcmd";
    }

    if (params) {
        argc = ARGS_Split(params, argv, ARRAY_SIZE(argv));
    }

    ctx.file = file;
    ctx.sec_prefix = sec_prefix;
    ctx.jit = jit;

    int ret = MYBPF_RunFile(&ctx, argc, (long)argv, 0, 0, 0);
    if (ret < 0) {
        printf("Run file failed. \r\n");
        ErrCode_Print();
        return ret;
    }

    return ctx.bpf_ret;
}

static int _runbpf_file(int argc, char **argv)
{
    static char *filename=NULL;
    static char *params = NULL;
    static char *sec_prefix = NULL;
    static GETOPT2_NODE_S opt[] = {
        {'o', 's', "section", 's', &sec_prefix, "section prefix", 0},
        {'o', 'j', "jit", 0, NULL, "jit", 0},
        {'o', 'p', "params", 's', &params, "params", 0},
        {'P', 0, "filename", 's', &filename, "bpf file name", 0},
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

    return _runbpf_run_file(filename, sec_prefix, jit, params);
}

static int _walk_prog_show(void *data, int len, char *sec_name, char *func_name, void *ud)
{
    printf("%s: %s \r\n", sec_name, func_name);
    return BS_WALK_CONTINUE;
}

static int _show_prog(int argc, char **argv)
{
    static char *filename=NULL;
    static GETOPT2_NODE_S opt[] = {
        {'P', 0, "filename", 's', &filename, "bpf file name", 0},
        {0} };

    if (BS_OK != GETOPT2_Parse(argc, argv, opt)) {
        _runbpf_opt_help(opt);
        return -1;
    }

    if (MYBPF_WalkProg(filename, _walk_prog_show, NULL) < 0) {
        printf("Can't process file %s \r\n", filename);
        return -1;
    }

    return 0;
}

static int _walk_prog_dump(void *data, int len, char *sec_name, char *func_name, void *ud)
{
    struct bpf_insn *code = data;
    int count = len / sizeof(struct bpf_insn);
    char *func = ud;
    int i;

    if ((func) && (strcmp(func, func_name) != 0)) {
        return BS_WALK_CONTINUE;
    }

    printf("%s: %s : \r\n", sec_name, func_name);

    for (i=0; i<count; i++) {
        UCHAR *d = (void*)&code[i];
        MEM_PrintCFormat(d, 8, NULL);
    }

    return BS_WALK_CONTINUE;
}

static int _dump_prog(int argc, char **argv)
{
    static char *filename=NULL;
    static char *function = NULL;
    static GETOPT2_NODE_S opt[] = {
        {'o', 'f', "function", 's', &function, "function name", 0},
        {'P', 0, "filename", 's', &filename, "bpf file name", 0},
        {0} };

    if (BS_OK != GETOPT2_Parse(argc, argv, opt)) {
        _runbpf_opt_help(opt);
        return -1;
    }

    if (MYBPF_WalkProg(filename, _walk_prog_dump, function) < 0) {
        printf("Can't process file %s \r\n", filename);
        return -1;
    }

    return 0;
}

static int _show_simple(int argc, char **argv)
{
    static char *filename=NULL;
    static GETOPT2_NODE_S opt[] = {
        {'P', 0, "filename", 's', &filename, "bpf file name", 0},
        {0} };
    char info[512] = "";

    if (BS_OK != GETOPT2_Parse(argc, argv, opt)) {
        _runbpf_opt_help(opt);
        return -1;
    }

    int ret = MYBPF_SIMPLE_BuildFileInfo(filename, info, sizeof(info));
    if (ret < 0) {
        ErrCode_Print();
    }

    printf("%s", info);

    return 0;
}

static int _walk_prog_export(void *data, int len, char *sec_name, char *func_name, void *ud)
{
    char *func = ud;
    FILE *fp;
    char buf[256];

    if ((func) && (strcmp(func, func_name) != 0)) {
        return BS_WALK_CONTINUE;
    }

    snprintf(buf, sizeof(buf), "%s.bpf", func_name);

    fp = FILE_Open(buf, TRUE, "wb+");
    if (! fp) {
        fprintf(stderr, "Can't open %s \r\n", buf);
        return BS_WALK_CONTINUE;
    }

    fwrite(data, 1, len, fp);

    FILE_Close(fp);

    return BS_WALK_CONTINUE;
}

static int _export_prog(int argc, char **argv)
{
    static char *filename=NULL;
    static char *function = NULL;
    static GETOPT2_NODE_S opt[] = {
        {'o', 'f', "function", 's', &function, "function name", 0},
        {'P', 0, "filename", 's', &filename, "bpf file name", 0},
        {0} };

    if (BS_OK != GETOPT2_Parse(argc, argv, opt)) {
        _runbpf_opt_help(opt);
        return -1;
    }

    if (MYBPF_WalkProg(filename, _walk_prog_export, function) < 0) {
        printf("Can't process file %s \r\n", filename);
        return -1;
    }

    return 0;
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
            ele_index = 0; /* 重置了offset, ele index重新计数 */
            base_offset = strtol(split + 1, NULL, 16);
            continue;
        }

        int imm = strtol(buf, NULL, 10);
        if (imm) {
            /* imm == 0 是空位 */
            map[index].imm = imm;
            map[index].new_imm = base_offset + ele_index * ele_size;
            index ++;
        }

        ele_index ++;
    }

    /* 最后一个写0表示结束 */
    map[index].imm = 0;

    fclose(fp);

    return map;
}

static int _convert_simple(int argc, char **argv)
{
    static char *filename=NULL;
    static char *output_name=NULL;
    static char *jit_arch_name =NULL;
    static char *convert_map_file=NULL;
    static GETOPT2_NODE_S opt[] = {
        {'P', 0, "filename", 's', &filename, "bpf file name", 0},
        {'o', 'j', "jit", 0, NULL, "jit", 0},
        {'o', 'a', "arch", 's', &jit_arch_name, "arch to jit", 0},
        {'o', 'm', "map", 's', &convert_map_file, "helper map file", 0},
        {'o', 'o', "output-name", 's', &output_name, "output name", 0},
        {0} };
    char simple_file[256];
    MYBPF_SIMPLE_CONVERT_PARAM_S p = {0};

    if (0 != GETOPT2_Parse(argc, argv, opt)) {
        _runbpf_opt_help(opt);
        return -1;
    }

    if (output_name) {
        strlcpy(simple_file, output_name, sizeof(simple_file));
    } else {
        snprintf(simple_file, sizeof(simple_file), "%s.spf", filename);
    }

    if (GETOPT2_IsOptSetted(opt, 'j', NULL)) {
        p.jit_arch = MYBPF_JIT_LocalArch();
    }

    if (jit_arch_name) {
        p.jit_arch = MYBPF_JIT_GetJitTypeByName(jit_arch_name);
        if (p.jit_arch == MYBPF_JIT_ARCH_NONE) {
            printf("Can't get arch %s \n", jit_arch_name);
            return -1;
        }
    }

    if (convert_map_file) {
        if (! (p.convert_calls_map = _build_convert_calls_map(convert_map_file))) {
            return -1;
        }
    } else {
        p.ext_call_agent = 1;
    }

    int ret = MYBPF_SIMPLE_ConvertBpf2Simple(filename, simple_file, &p);
    if (ret < 0) {
        ErrCode_Print();
    }

    MEM_FREE_NULL(p.convert_calls_map);

    return ret;
}

int main(int argc, char **argv)
{
    return SUBCMD_Do(g_subcmds, argc, argv);
}


