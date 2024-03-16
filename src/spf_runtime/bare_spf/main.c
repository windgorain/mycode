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
#include "utl/mybpf_bare.h"
#include "utl/mybpf_spf_def.h"
#include "utl/mybpf_loader_def.h"
#include "utl/mybpf_hookpoint_def.h"

static MYBPF_BARE_S g_mybpf_bare;

static void _runbpf_opt_help(GETOPT2_NODE_S *opt)
{
    char buf[4096];
    printf("%s", GETOPT2_BuildHelpinfo(opt, buf, sizeof(buf)));
    return;
}

static int _do_spf_cmd(int cmd, U64 p1, U64 p2, U64 p3, U64 p4)
{
    MYBPF_PARAM_S p;

    if (! g_mybpf_bare.prog) {
        fprintf(stderr, "Error: spf loader not loaded \n");
        return -1;
    }

    p.p[0] = cmd;
    p.p[1] = p1;
    p.p[2] = p2;
    p.p[3] = p3;
    p.p[4] = p4;

    int ret = MYBPF_RunBare(&g_mybpf_bare, NULL, &p);
    if (ret < 0) {
        fprintf(stderr, "Failed \n");
        return ret;
    }

    return 0;
}

static int _load_spf(int argc, char **argv)
{
    MYBPF_LOADER_PARAM_S p = {0};
    char *filename=NULL;
    char *instance=NULL;
    GETOPT2_NODE_S opt[] = {
        {'P', 0, "instance", GETOPT2_V_STRING, &instance, "instance name", 0},
        {'P', 0, "filename", GETOPT2_V_STRING, &filename, "bpf file name", 0},
        {0} };

    if (BS_OK != GETOPT2_Parse(argc, argv, opt)) {
        _runbpf_opt_help(opt);
        return -1;
    }

    p.instance = instance;
    p.filename = filename;
    p.flag = MYBPF_LOADER_FLAG_AUTO_ATTACH;

    return _do_spf_cmd(SPF_CMD_LOAD, (long)&p, 0, 0, 0);
}

static int _unload_spf(int argc, char **argv)
{
    char *instance = NULL;

    GETOPT2_NODE_S opt[] = {
        {'P', 0, "instance", GETOPT2_V_STRING, &instance, "instance name", 0},
        {0} };

    if (BS_OK != GETOPT2_Parse(argc, argv, opt)) {
        _runbpf_opt_help(opt);
        return -1;
    }

    return _do_spf_cmd(SPF_CMD_UNLOAD, (long)instance, 0, 0, 0);
}

static int _unload_all_spf(int argc, char **argv)
{
    return _do_spf_cmd(SPF_CMD_UNLOAD_ALL, 0, 0, 0, 0);
}

static int _run_spf_params(char *params)
{
    char *argv[32];
    int argc = 0;
    MYBPF_PARAM_S p = {0};

    if (params) {
        argc = ARGS_Split(params, argv, ARRAY_SIZE(argv));
    }

    p.p[0] = argc;
    p.p[1] = (long)argv;
 
    return _do_spf_cmd(SPF_CMD_RUN, MYBPF_HP_TCMD, (long)&p, 0, 0);
}

static int _test_spf_cmd(int argc, char **argv)
{
    char *params = NULL;
    GETOPT2_NODE_S opt[] = {
        {'o', 'p', "params", GETOPT2_V_STRING, &params, "params", 0},
        {0} };

    if (BS_OK != GETOPT2_Parse(argc, argv, opt)) {
        _runbpf_opt_help(opt);
        return -1;
    }

    return _run_spf_params(params);
}

static int _quit(int argc, char **argv)
{
    _do_spf_cmd(SPF_CMD_FIN, 0, 0, 0, 0);
    MYBPF_UnloadBare(&g_mybpf_bare);
    exit(0);
}

static int _run_spf(int argc, char **argv)
{
    static SUB_CMD_NODE_S subcmds[] = {
        {"load file", _load_spf, "load spf file"},
        {"unload instance", _unload_spf, "unload spf"},
        {"unload all", _unload_all_spf, "unload all spf"},
        {"testcmd", _test_spf_cmd, "test spf cmd"},
        {"quit", _quit, "quit"},
        {NULL}
    };

    return SUBCMD_DoParams(subcmds, argc, argv);
}

static int _load_spf_loader(char *spf_loader)
{
    int ret = MYBPF_LoadBareFile(spf_loader, NULL, &g_mybpf_bare);
    if (ret < 0) {
        ErrCode_PrintErrInfo();
        return ret;
    }

    _do_spf_cmd(SPF_CMD_INIT, 0, 0, 0, 0);

    return 0;
}

static int _run(char *spf_loader)
{
    char line[256];
    char *argv[32];
    int argc = 0;

    if (_load_spf_loader(spf_loader)) {
        return -1;
    }

	printf("> ");

    while(NULL != fgets(line, sizeof(line), stdin)){
        argc = ARGS_Split(line, argv, ARRAY_SIZE(argv));
        if (argc > 0) {
            _run_spf(argc, argv);
        }
        printf("> ");
    }

    return 0;
}


int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s spf_loader \n", argv[0]);
        return -1;
    }

    return _run(argv[1]);
}

