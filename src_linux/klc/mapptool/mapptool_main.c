/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include <linux/types.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/genetlink.h>
#include "bs.h"
#include "utl/ulimit_utl.h"
#include "utl/file_utl.h"
#include "utl/cff_utl.h"
#include "utl/elf_utl.h"
#include "utl/subcmd_utl.h"
#include "utl/getopt2_utl.h"
#include "utl/netlink_utl.h"
#include "utl/bpf_map.h"
#include "ko/ko_utl.h"
#include "klc/klc_def.h"
#include "klc/klc_map.h"
#include "klc/mapptool_lib.h"
#include "app/mapp/mapp_def.h"

static int _mapptool_cmd_load_instance(int argc, char **argv)
{
    char *file = NULL;
    char *instance = NULL;
    char *description = NULL;

    GETOPT2_NODE_S opts[] = {
        {'O', 'f', "file", 's', &file, "object file", 0},
        {'O', 'n', "instance", 's', &instance, "instance name", 0},
        {'o', 'd', "description", 's', &description, "description", 0},
        {0}
    };

	if (BS_OK != GETOPT2_Parse(argc, argv, opts)) {
        GETOPT2_PrintHelp(opts);
        return -1;
    }

    if (instance[0] == '_') { /* 保留字符, 用于保留instance name */
        BS_PRINT_ERR("instance name can't start by _ \n");
        return -1;
    }

    int ret = MAPPTOOL_Load(file, instance, description);
    if (ret < 0) {
        ErrCode_Print();
        return ret;
    }

    return 0;
}

static int _mapptool_cmd_unload_instance(int argc, char **argv)
{
    char *instance = NULL;

    GETOPT2_NODE_S opts[] = {
        {'O', 'n', "instance", 's', &instance, "instance name", 0},
        {0}
    };

	if (BS_OK != GETOPT2_Parse(argc, argv, opts)) {
        GETOPT2_PrintHelp(opts);
        return -1;
    }

    if (instance[0] == '_') { /* 保留字符, 用于保留instance name */
        BS_PRINT_ERR("instance name can't start by _ \n");
        return -1;
    }

    int ret = MAPPTOOL_UnLoad(instance);
    if (ret < 0) {
        ErrCode_Print();
        return ret;
    }

    return 0;
}

static int _mapptool_cmd_unload_all(int argc, char **argv)
{
    char *instance = NULL;

    DIR_SCAN_START(MAPPTOOL_PIN_PATH, instance) {
        MAPPTOOL_UnLoad(instance);
    }DIR_SCAN_END();

    return 0;
}

static int _mapptool_cmd_attach_xdp(int argc, char **argv)
{
    char *prog = NULL;
    char *ifname = NULL;
    char *instance = NULL;

    GETOPT2_NODE_S opts[] = {
        {'O', 'n', "instance", 's', &instance, "instance name", 0},
        {'O', 't', "to", 's', &ifname, "attach to the ifname", 0},
        {'O', 'p', "prog", 's', &prog, "prog name", 0},
        {0}
    };

	if (BS_OK != GETOPT2_Parse(argc, argv, opts)) {
        GETOPT2_PrintHelp(opts);
        return -1;
    }

    int ret = MAPPTOOL_AttachXdp(instance, prog, ifname);
    if (ret < 0) {
        ErrCode_Print();
        return ret;
    }

    return 0;
}

static int _mapptool_cmd_attach_instance(int argc, char **argv)
{
    char *prog = NULL;
    char *attach_to = NULL;
    char *instance = NULL;
    int slot = -1;

    GETOPT2_NODE_S opts[] = {
        {'O', 'n', "instance", 's', &instance, "instance name", 0},
        {'O', 't', "to", 's', &attach_to, "attach to the instance", 0},
        {'O', 'p', "prog", 's', &prog, "prog name", 0},
        {'o', 's', "slot", 'u', &slot, "slot, dft: auto", 0},
        {0}
    };

	if (BS_OK != GETOPT2_Parse(argc, argv, opts)) {
        GETOPT2_PrintHelp(opts);
        return -1;
    }

    if (strcmp(attach_to, instance) == 0) {
        fprintf(stderr, "Err: can't attach self");
        return -1;
    }

    int ret = MAPPTOOL_AttachInstance(instance, prog, attach_to, slot);
    if (ret < 0) {
        ErrCode_Print();
        return ret;
    }

    return 0;
}

static int _mapptool_cmd_detach_xdp(int argc, char **argv)
{
    char *ifname = NULL;

    GETOPT2_NODE_S opts[] = {
        {'O', 'f', "form", 's', &ifname, "ifname to detach", 0},
        {0}
    };

	if (BS_OK != GETOPT2_Parse(argc, argv, opts)) {
        GETOPT2_PrintHelp(opts);
        return -1;
    }

    int ret = MAPPTOOL_DetachXdp(ifname);
    if (ret < 0) {
        ErrCode_Print();
        return ret;
    }

    return 0;
}

static int _mapptool_cmd_detach_instance(int argc, char **argv)
{
    char *instance = NULL;
    char *detach_from = NULL;

    GETOPT2_NODE_S opts[] = {
        {'O', 'n', "instance", 's', &instance, "instance name", 0},
        {'O', 'f', "from", 's', &detach_from, "detach from", 0},
        {0}
    };

	if (BS_OK != GETOPT2_Parse(argc, argv, opts)) {
        GETOPT2_PrintHelp(opts);
        return -1;
    }

    int ret = MAPPTOOL_DetachInstance(instance, detach_from);
    if (ret < 0) {
        ErrCode_Print();
        return ret;
    }

    return 0;
}

/* 从指定的instance中detach掉所有solt */
static int _mapptool_cmd_detach_instance_all(int argc, char **argv)
{
    char *detach_from = NULL;

    GETOPT2_NODE_S opts[] = {
        {'O', 'f', "from", 's', &detach_from, "detach from", 0},
        {0}
    };

	if (BS_OK != GETOPT2_Parse(argc, argv, opts)) {
        GETOPT2_PrintHelp(opts);
        return -1;
    }

    int ret = MAPPTOOL_DetachInstanceAll(detach_from);
    if (ret < 0) {
        ErrCode_Print();
        return ret;
    }

    return 0;
}

static int _mapptool_cmd_show_slot(int argc, char **argv)
{
    char *instance = NULL;
    int slot_num;
    int i;
    int ret;
    MAPP_SLOT_DATA_S data;

    GETOPT2_NODE_S opts[] = {
        {'O', 'n', "instance", 's', &instance, "instance name", 0},
        {0}
    };

	if (BS_OK != GETOPT2_Parse(argc, argv, opts)) {
        GETOPT2_PrintHelp(opts);
        return -1;
    }

    slot_num = MAPPTOOL_GetSlotNum(instance);
    if (slot_num < 0) {
        BS_PRINT_ERR("Can't get traffic slot number \n");
        return slot_num;
    }

    for (i=0; i<slot_num; i++) {
        ret = MAPPTOOL_GetSlotData(instance, i, &data);
        if ((ret >= 0) && (data.attached_name[0])){
            printf(" slot:%u, instance:%s \n", i, data.attached_name);
        }
    }

    return 0;
}

static int _mapptool_cmd_show_attr(int argc, char **argv)
{
    char *instance = NULL;
    MAPP_ATTR_S attr = {0};
    int ret;

    GETOPT2_NODE_S opts[] = {
        {'O', 'n', "instance", 's', &instance, "instance name", 0},
        {0}
    };

	if (BS_OK != GETOPT2_Parse(argc, argv, opts)) {
        GETOPT2_PrintHelp(opts);
        return -1;
    }

    ret = MAPPTOOL_GetAttr(instance, &attr);
    if (ret < 0) {
        ErrCode_Print();
        return ret;
    }

    printf("class: %s\n", MAPPTOOL_Class2String(attr.class));
    printf("in_type: %s\n", MAPPTOOL_Type2String(attr.in_type));
    printf("out_type: %s\n", MAPPTOOL_Type2String(attr.out_type));
    printf("mapp: %s\n", attr.mapp_name);
    printf("instance: %s\n", attr.instance_name);
    printf("description: %s\n", attr.description);

    return 0;
}

static int _mapptool_cmd_show_instance(int argc, char **argv)
{
    char *instance = NULL;

    DIR_SCAN_START(MAPPTOOL_PIN_PATH, instance) {
        printf("%s \n", instance);
    }DIR_SCAN_END();

    return 0;
}

int main(int argc, char **argv)
{
    static SUB_CMD_NODE_S subcmds[] = {
        {"load instance", _mapptool_cmd_load_instance, "load instance"},
        {"unload instance", _mapptool_cmd_unload_instance, "unload instance"},
        {"unload all-instance", _mapptool_cmd_unload_all, "unload all instance"},
        {"attach xdp", _mapptool_cmd_attach_xdp, "attach to xdp"},
        {"detach xdp", _mapptool_cmd_detach_xdp, "detach from xdp"},
        {"attach instance", _mapptool_cmd_attach_instance, "attach to instance"},
        {"detach instance", _mapptool_cmd_detach_instance, "detach from instance"},
        {"detach all-instance", _mapptool_cmd_detach_instance_all, "detach all from instance"},
        {"show instance", _mapptool_cmd_show_instance, "show loaded instance"},
        {"show slot", _mapptool_cmd_show_slot, "show instance slot"},
        {"show attr", _mapptool_cmd_show_attr, "show instance attr"},
        {NULL, NULL}
    };

    ULIMIT_SetMemLock(RLIM_INFINITY, RLIM_INFINITY);

    return SUBCMD_Do(subcmds, argc, argv);
}

