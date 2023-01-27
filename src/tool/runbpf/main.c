/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/args_utl.h"
#include "utl/subcmd_utl.h"
#include "utl/getopt2_utl.h"
#include "utl/mybpf_prog.h"
#include "utl/mybpf_loader.h"
#include "utl/mybpf_vm.h"
#include "utl/mybpf_elf.h"
#include "utl/mybpf_file.h"
#include "utl/file_utl.h"

static int _runbpf_file(int argc, char **argv);
static int _dump_prog(int argc, char **argv);
static int _show_prog(int argc, char **argv);
static int _export_prog(int argc, char **argv);

static SUB_CMD_NODE_S g_subcmds[] = 
{
    {"run", _runbpf_file, "run file"},
    {"dump prog", _dump_prog, "dump prog"},
    {"show prog", _show_prog, "show prog"},
    {"export prog", _export_prog, "export prog"},
    {NULL, NULL}
};

static void _runbpf_opt_help(GETOPT2_NODE_S *opt)
{
    char buf[512];
    printf("%s", GETOPT2_BuildHelpinfo(opt, buf, sizeof(buf)));
    return;
}

static int _runbpf_run_file(char *file, char *func, char *params)
{
    char *argv[32];
    int argc = 0;
    UINT64 bpf_ret;

    if (! func) {
        func = "main";
    }

    if (params) {
        argc = ARGS_Split(params, argv, ARRAY_SIZE(argv));
    }

    int ret = MYBPF_RunFile(file, func, &bpf_ret, argc, (long)argv, 0, 0, 0);
    if (ret < 0) {
        printf("Run file failed. \r\n");
        ErrCode_Print();
        return ret;
    }

    return bpf_ret;
}

static int _runbpf_file(int argc, char **argv)
{
    static char *filename=NULL;
    static char *params = NULL;
    static char *function = NULL;
    static GETOPT2_NODE_S opt[] = {
        {'o', 'f', "function", 's', &function, "function name", 0},
        {'o', 'p', "params", 's', &params, "params", 0},
        {'P', 0, "filename", 's', &filename, "bpf file name", 0},
        {0} };

    if (BS_OK != GETOPT2_Parse(argc, argv, opt)) {
        _runbpf_opt_help(opt);
        return -1;
    }

    if (filename == NULL) {
        _runbpf_opt_help(opt);
        return -1;
    }

    return _runbpf_run_file(filename, function, params);
}

static BS_WALK_RET_E _walk_prog_show(void *data, int len, char *sec_name, char *func_name, void *ud)
{
    printf("%s: %s \r\n", sec_name, func_name);
    return BS_WALK_CONTINUE;
}

static BS_WALK_RET_E _walk_prog_dump(void *data, int len, char *sec_name, char *func_name, void *ud)
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
        if (i+1 == count) {
            printf("0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x \r\n", 
                    d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7]);
        } else {
            printf("0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, \r\n", 
                    d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7]);
        }
    }

    return BS_WALK_CONTINUE;
}

static BS_WALK_RET_E _walk_prog_export(void *data, int len, char *sec_name, char *func_name, void *ud)
{
    char *func = ud;
    FILE *fp;

    if ((func) && (strcmp(func, func_name) != 0)) {
        return BS_WALK_CONTINUE;
    }

    fp = FILE_Open(func_name, TRUE, "wb+");
    if (! fp) {
        fprintf(stderr, "Can't open %s \r\n", func_name);
        return BS_WALK_CONTINUE;
    }

    fwrite(data, 1, len, fp);

    FILE_Close(fp);

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

int main(int argc, char **argv)
{
    return SUBCMD_Do(g_subcmds, argc, argv);
}


