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
#include "utl/bpf_map.h"
#include "ko/ko_utl.h"
#include "klc/klc_def.h"
#include "klc/klc_map.h"
#include "klc/klctool_def.h"
#include "klc/klctool_lib.h"

/* load filename */
static int _klctool_cmd_load_file(int argc, char **argv)
{
    char *file = NULL;
    int replace = 0;

    /* -r 和reload相比,并不删除module,只将file中的数据添加/更新原来的部分数据. 比如用于单个函数的升级等 */
    GETOPT2_NODE_S opts[] = {
        {'P', 0, "File", 's', &file, "file name", 0},
        {'o', 'r', "replace", 0, NULL, "replace if exist", 0}, 
        {0}
    };

	if (0 != GETOPT2_Parse(argc, argv, opts)) {
        GETOPT2_PrintHelp(opts);
        return -1;
    }

    if (GETOPT2_IsOptSetted(opts, 'r', NULL)) {
        replace = 1;
    }

    return KLCTOOL_LoadFile(file, replace, NULL, NULL);
}

/* unload filename */
static int _klctool_cmd_unload_file(int argc, char **argv)
{
    char *file = NULL;

    GETOPT2_NODE_S opts[] = {
        {'P', 0, "File", 's', &file, "file name", 0},
        {0}
    };

	if (0 != GETOPT2_Parse(argc, argv, opts)) {
        GETOPT2_PrintHelp(opts);
        return -1;
    }

    return KLCTOOL_UnLoadFile(file);
}

/* reload filename */
static int _klctool_cmd_reload_file(int argc, char **argv)
{
    char *file = NULL;

    GETOPT2_NODE_S opts[] = {
        {'P', 0, "File", 's', &file, "file name", 0},
        {0}
    };

	if (0 != GETOPT2_Parse(argc, argv, opts)) {
        GETOPT2_PrintHelp(opts);
        return -1;
    }

    if (file == NULL) {
        GETOPT2_PrintHelp(opts);
        return -1;
    }

    KLCTOOL_UnLoadFile(file);
    return KLCTOOL_LoadFile(file, 0, NULL, NULL);
}

static int _klctool_cmd_del_module(int argc, char **argv)
{
    char *str = NULL;

    GETOPT2_NODE_S opts[] = {
        {'P', 0, "Name", 's', &str, "module name", 0},
        {0}
    };

	if (0 != GETOPT2_ParseFromArgv0(argc-1, argv+1, opts)) {
        GETOPT2_PrintHelp(opts);
        return -1;
    }

    return KLCTOOL_DelModule(str);
}

static int _klctool_cmd_del_idfunc(int argc, char **argv)
{
    int id = -1;

    GETOPT2_NODE_S opts[] = {
        {'P', 0, "ID", 'u', &id, "function id", 0},
        {0}
    };

	if (0 != GETOPT2_ParseFromArgv0(argc-1, argv+1, opts)) {
        GETOPT2_PrintHelp(opts);
        return -1;
    }

    return KLCTOOL_DelIDFunc(id);
}

static int _klctool_cmd_del_namefunc(int argc, char **argv)
{
    char *str = NULL;

    GETOPT2_NODE_S opts[] = {
        {'P', 0, "Name", 's', &str, "function name", 0},
        {0}
    };

	if (0 != GETOPT2_ParseFromArgv0(argc-1, argv+1, opts)) {
        GETOPT2_PrintHelp(opts);
        return -1;
    }

    return KLCTOOL_DelNameFunc(str);
}

static int _klctool_cmd_del_evob(int argc, char **argv)
{
    int event = -1;
    char *name = NULL;

    GETOPT2_NODE_S opts[] = {
        {'P', 0, "ID", 'u', &event, "event id", 0},
        {'P', 0, "Name", 's', &name, "evob name", 0},
        {0}
    };

	if (0 != GETOPT2_ParseFromArgv0(argc-1, argv+1, opts)) {
        GETOPT2_PrintHelp(opts);
        return -1;
    }

    return KLCTOOL_DelEvob(event, name);
}

static int _klctool_cmd_show_module(int argc, char **argv)
{
    return KLCTOOL_ShowModule();
}

static int _klctool_cmd_show_idfunc(int argc, char **argv)
{
    int id = -1;

    GETOPT2_NODE_S opts[] = {
        {'p', 0, "ID", 'u', &id, "function id", 0},
        {0}
    };

	if (0 != GETOPT2_Parse(argc, argv, opts)) {
        GETOPT2_PrintHelp(opts);
        return -1;
    }

    return KLCTOOL_ShowIDFunc(id);
}

static int _klctool_cmd_show_namefunc(int argc, char **argv)
{
    char *key = NULL;

    GETOPT2_NODE_S opts[] = {
        {'p', 0, "Name", 's', &key, "function name", 0},
        {0}
    };

	if (0 != GETOPT2_Parse(argc, argv, opts)) {
        GETOPT2_PrintHelp(opts);
        return -1;
    }

    return KLCTOOL_ShowNameFunc(key);
}

static int _klctool_cmd_show_map(int argc, char **argv)
{
    return KLCTOOL_ShowNameMap();
}

static int _klctool_cmd_show_evob(int argc, char **argv)
{
    return KLCTOOL_ShowEvob();
}

static int _klctool_cmd_show_oshelper(int argc, char **argv)
{
    return KLCTOOL_ShowOsHelper();
}

static int _klctool_cmd_dump(int argc, char **argv)
{
    char *type = NULL;
    char *name = NULL;
    int id = -1;
    int ret = -1;

    GETOPT2_NODE_S opts[] = {
        {'P', 0, "Type", 's', &type, "function type, one of id/name", 0},
        {'P', 0, "Key", 's', &name, "function id/name", 0},
        {0}
    };

	if (0 != GETOPT2_ParseFromArgv0(argc-1, argv+1, opts)) {
        GETOPT2_PrintHelp(opts);
        return -1;
    }

    if (strcmp(type, "id") == 0) {
        id = TXT_Str2Ui(name);
        ret = KLCTOOL_DumpIDFunc(id);
    } else if (strcmp(type, "name") == 0) {
        ret = KLCTOOL_DumpNameFunc(name);
    } else {
        BS_PRINT_ERR("type %s not support \n", type);
        GETOPT2_PrintHelp(opts);
    }

    return ret;
}

static int _klctool_cmd_decuse_module(int argc, char **argv)
{
    return KLCTOOL_DecUse();
}

static int _klctool_cmd_incuse_module(int argc, char **argv)
{
    return KLCTOOL_IncUse();
}

static int _klctool_self_check()
{
    if (BS_OFFSET(KLC_EVENT_OB_S, hdr) != 0) {
        return -1;
    }

    if (BS_OFFSET(KLC_FUNC_S, hdr) != 0) {
        return -1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    static SUB_CMD_NODE_S subcmds[] = {
        {"load", _klctool_cmd_load_file, "load file"},
        {"unload", _klctool_cmd_unload_file, "unload file"},
        {"reload", _klctool_cmd_reload_file, "reload file"},
        {"delete module", _klctool_cmd_del_module, "delete module"},
        {"delete idfunc", _klctool_cmd_del_idfunc, "delete id function"},
        {"delete namefunc", _klctool_cmd_del_namefunc, "delete name function"},
        {"delete evob", _klctool_cmd_del_evob, "delete event ob"},
        {"show module", _klctool_cmd_show_module, "show module"},
        {"show idfunc", _klctool_cmd_show_idfunc, "show id function"},
        {"show namefunc", _klctool_cmd_show_namefunc, "show name function"},
        {"show map", _klctool_cmd_show_map, "show name map"},
        {"show evob", _klctool_cmd_show_evob, "show event ob"},
        {"show oshelper", _klctool_cmd_show_oshelper, "show os helper"},
        {"dump function", _klctool_cmd_dump, "dump function"},
        {"decuse", _klctool_cmd_decuse_module, "decuse module"},
        {"incuse", _klctool_cmd_incuse_module, "incuse module"},
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

    if (_klctool_self_check() < 0) {
        BS_PRINT_ERR("Error: Self check error\n");
        return -1;
    }

    return SUBCMD_Do(subcmds, argc, argv);
}

