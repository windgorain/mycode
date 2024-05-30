/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include <stdio.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <linux/types.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/genetlink.h>
#include "bs.h"
#include "utl/cff_utl.h"
#include "utl/ulimit_utl.h"
#include "utl/elf_utl.h"
#include "utl/subcmd_utl.h"
#include "utl/getopt2_utl.h"
#include "utl/netlink_utl.h"
#include "utl/txt_utl.h"
#include "utl/bpf_map.h"
#include "ko/ko_utl.h"
#include "klc/klc_def.h"
#include "klc/klc_map.h"
#include "klc/klctool_def.h"
#include "klc/klctool_lib.h"

static int _klctool_init_klc(int argc, char **argv)
{
    return KLCTOOL_InitKlc();
}

static int _klctool_cmd_load_bare_file(int argc, char **argv)
{
    char *file = NULL;

    GETOPT2_NODE_S opts[] = {
        {'P', 0, "File", GETOPT2_V_STRING, &file, "file name", 0},
        {0}
    };

	if (0 != GETOPT2_Parse(argc, argv, opts)) {
        ErrCode_PrintErrInfo();
        GETOPT2_PrintHelp(opts);
        return -1;
    }

    return KLCTOOL_LoadBareFile(file);
}

static int _klctool_cmd_load_spf_file(int argc, char **argv)
{
    char *file = NULL;
    char *instance = NULL;

    GETOPT2_NODE_S opts[] = {
        {'P', 0, "File", GETOPT2_V_STRING, &file, "file name", 0},
        {'o', 'i', "instance", GETOPT2_V_STRING, &instance, "instance name", 0},
        {0}
    };

	if (0 != GETOPT2_Parse(argc, argv, opts)) {
        ErrCode_PrintErrInfo();
        GETOPT2_PrintHelp(opts);
        return -1;
    }

    return KLCTOOL_LoadSpfFile(file, instance);
}

static int _klctool_cmd_unload_instance(int argc, char **argv)
{
    char *str = NULL;

    GETOPT2_NODE_S opts[] = {
        {'P', 0, "Instance", GETOPT2_V_STRING, &str, "instance name", 0},
        {0}
    };

	if (0 != GETOPT2_Parse(argc, argv, opts)) {
        ErrCode_PrintErrInfo();
        GETOPT2_PrintHelp(opts);
        return -1;
    }

    if (KLCTOOL_UnloadInstance(str) < 0) {
        fprintf(stderr, "can't unload instance %s \n", str);
        return -1;
    }

    return 0;
}

static int _klctool_cmd_show_instance(int argc, char **argv)
{
    return KLCTOOL_ShowInstance();
}

static int _klctool_cmd_run_cmd(int argc, char **argv)
{
    return KLCTOOL_RunCmd(argc, argv);
}

static int _klctool_cmd_show_idfunc(int argc, char **argv)
{
    return KLCTOOL_ShowIDFunc();
}

static int _klctool_cmd_show_namefunc(int argc, char **argv)
{
    return KLCTOOL_ShowNameFunc();
}

static int _klctool_cmd_show_evob(int argc, char **argv)
{
    return KLCTOOL_ShowEvob();
}

static int _klctool_cmd_decuse_module(int argc, char **argv)
{
    return KLCTOOL_DecUse();
}

static int _klctool_cmd_incuse_module(int argc, char **argv)
{
    return KLCTOOL_IncUse();
}

static int _klctool_cmd_decuse_base(int argc, char **argv)
{
    return KLCTOOL_DecBaseUse();
}

static int _klctool_cmd_incuse_base(int argc, char **argv)
{
    return KLCTOOL_IncBaseUse();
}

int main(int argc, char **argv)
{
    static SUB_CMD_NODE_S subcmds[] = {
        {"init", _klctool_init_klc, "init klc"},
        {"load bare", _klctool_cmd_load_bare_file, "load bare file"},
        {"load spf", _klctool_cmd_load_spf_file, "load spf file"},
        {"unload instance", _klctool_cmd_unload_instance, "unload instance"},
        {"show instance", _klctool_cmd_show_instance, "show instance"},
        {"cmdrun", _klctool_cmd_run_cmd, "run cmd"},

        {"show idfunc", _klctool_cmd_show_idfunc, "show id function"},
        {"show namefunc", _klctool_cmd_show_namefunc, "show name function"},
        {"show evob", _klctool_cmd_show_evob, "show event ob"},

        
        {"decuse impl", _klctool_cmd_decuse_module, "decuse module"},
        {"incuse impl", _klctool_cmd_incuse_module, "incuse module"},
        {"decuse base", _klctool_cmd_decuse_base, "decuse module"},
        {"incuse base", _klctool_cmd_incuse_base, "incuse module"},
        {NULL}
    };

    ULIMIT_SetMemLock(RLIM_INFINITY, RLIM_INFINITY);

    int ko_version = KLCTOOL_KoVersion();
    if (ko_version < 0) {
        BS_PRINT_ERR("Error: Get version errror \n");
    }

    if (ko_version != KLC_KO_VERSION) {
        BS_PRINT_ERR("Error: klctool version %d not match ko version %d \n", KLC_KO_VERSION, ko_version);
        return -1;
    }

    return SUBCMD_Do(subcmds, argc, argv);
}

