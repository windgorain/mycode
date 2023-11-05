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


static int _klctool_cmd_load_file(int argc, char **argv)
{
    char *file = NULL;
    KLCTOOL_LOAD_CFG_S load_cfg = {0};

    
    GETOPT2_NODE_S opts[] = {
        {'P', 0, "File", GETOPT2_V_STRING, &file, "file name", 0},
        {'o', 'r', "replace", GETOPT2_V_NONE, NULL, "replace if exist", 0}, 
        {0}
    };

	if (0 != GETOPT2_Parse(argc, argv, opts)) {
        ErrCode_PrintErrInfo();
        GETOPT2_PrintHelp(opts);
        return -1;
    }

    if (GETOPT2_IsOptSetted(opts, 'r', NULL)) {
        load_cfg.replace = 1;
    }

    return KLCTOOL_LoadFile(file, NULL, &load_cfg);
}


static int _klctool_cmd_unload_file(int argc, char **argv)
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

    if (KLCTOOL_UnLoadFile(file) < 0) {
        fprintf(stderr, "can't unload the file\n");
        return -1;
    }

    return 0;
}


static int _klctool_cmd_reload_file(int argc, char **argv)
{
    char *file = NULL;
    KLCTOOL_LOAD_CFG_S load_cfg = {0};

    GETOPT2_NODE_S opts[] = {
        {'P', 0, "File", GETOPT2_V_STRING, &file, "file name", 0},
        {0}
    };

	if (0 != GETOPT2_Parse(argc, argv, opts)) {
        ErrCode_PrintErrInfo();
        GETOPT2_PrintHelp(opts);
        return -1;
    }

    if (file == NULL) {
        GETOPT2_PrintHelp(opts);
        return -1;
    }

    KLCTOOL_UnLoadFile(file);

    return KLCTOOL_LoadFile(file, NULL, &load_cfg);
}

static int _klctool_cmd_cmdrun(int argc, char **argv)
{
    char *mod = NULL;
    char *cmd = NULL;

    GETOPT2_NODE_S opts[] = {
        {'O', 'm', "mod", GETOPT2_V_STRING, &mod, "module name", 0},
        {'P', 0, "cmd", GETOPT2_V_STRING, &cmd, "run cmd", 0},
        {0}
    };

	if (0 != GETOPT2_Parse(argc, argv, opts)) {
        ErrCode_PrintErrInfo();
        GETOPT2_PrintHelp(opts);
        return -1;
    }

    return KLCTOOL_CmdRun(mod, cmd);
}

static int _klctool_cmd_del_module(int argc, char **argv)
{
    char *str = NULL;

    GETOPT2_NODE_S opts[] = {
        {'P', 0, "Name", GETOPT2_V_STRING, &str, "module name", 0},
        {0}
    };

	if (0 != GETOPT2_Parse(argc, argv, opts)) {
        ErrCode_PrintErrInfo();
        GETOPT2_PrintHelp(opts);
        return -1;
    }

    if (KLCTOOL_DelModule(str) < 0) {
        fprintf(stderr, "can't delete the mod\n");
        return -1;
    }

    return 0;
}

static int _klctool_cmd_show_module(int argc, char **argv)
{
    return KLCTOOL_ShowModule();
}

static int _klctool_cmd_show_idfunc(int argc, char **argv)
{
    return KLCTOOL_ShowIDFunc();
}

static int _klctool_cmd_show_namefunc(int argc, char **argv)
{
    return KLCTOOL_ShowNameFunc();
}

static int _klctool_cmd_show_name_map(int argc, char **argv)
{
    return KLCTOOL_ShowNameMap();
}

static int _klctool_cmd_show_maps(int argc, char **argv)
{
    char *mod_name = NULL;

    GETOPT2_NODE_S opts[] = {
        {'o', 'm', "module", GETOPT2_V_STRING, &mod_name, "module name", 0}, 
        {0}
    };

	if (0 != GETOPT2_Parse(argc, argv, opts)) {
        ErrCode_PrintErrInfo();
        GETOPT2_PrintHelp(opts);
        return -1;
    }

    return KLCTOOL_ShowMaps(mod_name);
}

static int _klctool_cmd_show_evob(int argc, char **argv)
{
    return KLCTOOL_ShowEvob();
}

static int _klctool_cmd_show_oshelper(int argc, char **argv)
{
    return KLCTOOL_ShowOsHelper();
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
        {"load", _klctool_cmd_load_file, "load file"},
        {"unload", _klctool_cmd_unload_file, "unload file"},
        {"reload", _klctool_cmd_reload_file, "reload file"},
        {"cmdrun", _klctool_cmd_cmdrun, "cmd run"},
        {"delete module", _klctool_cmd_del_module, "delete module"},
        {"show module", _klctool_cmd_show_module, "show module"},
        {"show idfunc", _klctool_cmd_show_idfunc, "show id function"},
        {"show namefunc", _klctool_cmd_show_namefunc, "show name function"},
        {"show namemap", _klctool_cmd_show_name_map, "show name map"},
        {"show maps", _klctool_cmd_show_maps, "show maps"},
        {"show evob", _klctool_cmd_show_evob, "show event ob"},
        {"show oshelper", _klctool_cmd_show_oshelper, "show os helper"},
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

