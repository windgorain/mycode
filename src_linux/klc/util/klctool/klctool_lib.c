/*================================================================
*   Copyright LiXingang
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
#include "utl/elf_utl.h"
#include "utl/ubpf_utl.h"
#include "utl/ebpf_utl.h"
#include "utl/subcmd_utl.h"
#include "utl/getopt2_utl.h"
#include "utl/bit_opt.h"
#include "utl/netlink_utl.h"
#include "utl/bpf_map.h"
#include "ko/ko_utl.h"
#include "klc/klc_def.h"
#include "klc/klc_map.h"
#include "klc/klctool_def.h"
#include "klc/klctool_lib.h"
#ifdef USE_KLCTOOL_MAPFUNC
#include <bpf/bpf.h>
#endif

#define KLCTOOL_SHOW_MAX  1024
#define KLCTOOL_DUMP_SIZE 4096
#define KLCTOOL_SEC_PREFIX "klc"
#define KLCTOOL_SEC_MODULE KLCTOOL_SEC_PREFIX "/module"
#define KLCTOOL_SEC_FUNC_ATTR KLCTOOL_SEC_PREFIX "/func_attr"

typedef struct {
    char *file;
    char *module_name;
    char *map_pin_path;
    int replace;
}KLCTOOL_LOAD_S;

typedef struct {
    unsigned int replace:1;
    int cmd;
    int tok_num;
    char **tok;
    char *file;
    char *map_pin_path;
    KLC_MODULE_INFO_S *module;
    NETLINK_S *nl;
    ELF_S *elf;
    ELF_SECTION_S *sec;
}KLCTOOL_LOAD_INFO_S;

static KLC_BPF_HEADER_S g_klctool_insn_hdr = {.ver = 1, .flag=1};
static char *g_klctool_ko_err_msg[] = {
    "base",
#define _(a,b) b,
    KO_ERR_DEF
#undef _
    "unknown"
};

static KLC_OSHELPER_LIST_S g_klctool_oshelpers = {0};

static char * _klctool_get_ko_err_msg(int ret)
{
    int index = ret - KO_ERR_BASE;

    if ((ret >= KO_ERR_MAX) || (ret <= KO_ERR_BASE)) {
        return "unknown";
    }

    return g_klctool_ko_err_msg[index];
}

static int _klctool_send_nl_msg(NETLINK_S *nl, int cmd, void *data, int data_len, void *out)
{
    int ret;

    ret = NetLink_SendMsg(nl, cmd, data, data_len, out);
    if (ret < 0) {
        if (ret > KO_ERR_MAX) {
            fprintf(stderr, "nl msg error %d \n", ret);
        }
    }

    return ret;
}

static int _klctool_common_cmd(NETLINK_S *nl, int cmd, void *data, int data_len)
{
    return _klctool_send_nl_msg(nl, cmd, data, data_len, NULL);
}

static inline int _klctool_get_os_helper(NETLINK_S *nl, OUT KLC_OSHELPER_LIST_S *list)
{
    int cmd = KLC_NL_TYPE_CMD(KLC_NL_TYPE_SYS, KLC_NL_SYS_GET_OS_HELPER);
    return _klctool_send_nl_msg(nl, cmd, NULL, 0, list);
}

static int _klctool_check_name_char(char c)
{
    if (c >= 'A' && c <= 'Z') {
        return 0;
    } else if (c >= 'a' && c <= 'z') {
        return 0;
    } else if (c >= '0' && c <= '9') {
        return 0;
    } else if ((c == '_') || (c == '/')){
        return 0;
    }

    return -1;
}

static int _klctool_check_name(char *name)
{
    char *c;

    if (! name) {
        return -1;
    }

    if (name[0] == '\0') {
        return -1;
    }

    c = name;
    while (*c) {
        if (_klctool_check_name_char(*c) < 0) {
            return -1;
        }
        c++;
    }

    return 0;
}

static int  _klctool_get_sec(ELF_S *elf, char *sec_name, OUT ELF_SECTION_S *sec)
{
    return ELF_GetSecByName(elf, sec_name, sec);
}

static KLC_FUNC_ATTR_S * _klctool_get_func_attr(ELF_S *elf, char *funcname)
{
    ELF_SECTION_S sec;
    KLC_FUNC_ATTR_S *func;
    int data_len, count;
    int i;

    if (_klctool_get_sec(elf, KLCTOOL_SEC_FUNC_ATTR, &sec) < 0) {
        return NULL;
    }

    func = sec.data->d_buf;
    data_len = sec.data->d_size;
    count = data_len / sizeof(KLC_FUNC_ATTR_S);

    for (i=0; i<count; i++, func++) {
        if (strcmp(func->func, funcname) == 0) {
            return func;
        }
    }

    return NULL;
}

static KLC_FUNC_S * _klctool_alloc_idname_func(void *data, int data_len,
        KLC_FUNC_ATTR_S *attr, char *module_name, char *name)
{
    KLC_FUNC_S *func = NULL;
    int len;

    len = sizeof(KLC_FUNC_S) + data_len + KLC_BPF_HEADER_SIZE;

    func = MEM_ZMalloc(len);
    if (! func) {
        fprintf(stderr, "Can't alloc function %s memory \n", name);
        return NULL;
    }

    int nlen = SNPRINTF(func->hdr.name, sizeof(func->hdr.name), "%s.%s", module_name, name);
    if (nlen < 0) {
        fprintf(stderr, "%s.%s too long\n", module_name, name);
        MEM_Free(func);
        return NULL;
    }

    func->hdr.size = len;
    func->insn_len = data_len + KLC_BPF_HEADER_SIZE;
    memcpy(func->insn, &g_klctool_insn_hdr, sizeof(g_klctool_insn_hdr));
    memcpy(&func->insn[sizeof(g_klctool_insn_hdr)], data, data_len);

    return func;
}

static KLC_FUNC_S * _klctool_idname_func_jit(void *data, int data_len,
        KLC_FUNC_ATTR_S *attr, char *module_name, char *name)
{
    UBPF_VM_HANDLE vm = NULL;
    ubpf_jit_fn fn = NULL;
    KLC_FUNC_S *func;
    void *jdata = data;
    int len = data_len;

    vm = UBPF_CreateLoad(data, len, g_klctool_oshelpers.helpers, KLC_OSHELPER_MAX);
    if (vm) {
        fn = UBPF_E2j(vm);
        if (fn) {
            jdata = fn;
            len = UBPF_GetJittedSize(vm);
        }
    }

    func = _klctool_alloc_idname_func(jdata, len, attr, module_name, name);

    if (func && fn) {
        func->hdr.exe = 1;
    }

    if (vm) {
        UBPF_Destroy(vm);
    }

    return func;
}

static int _klctool_add_func(void *nl, void *data, int data_len,
        KLC_FUNC_ATTR_S *attr, char *module_name, int type, int id, char *name, int replace)
{
    KLC_FUNC_S *func = NULL;
    int nlcmd;

    if (type == KLC_FUNC_TYPE_NAME) {
        nlcmd = KLC_NL_TYPE_CMD(KLC_NL_TYPE_NAME_FUNC, KLC_NL_FUNC_ADD);
    } else if (type == KLC_FUNC_TYPE_ID) {
        nlcmd = KLC_NL_TYPE_CMD(KLC_NL_TYPE_ID_FUNC, KLC_NL_FUNC_ADD);
    } else {
        return -1;
    }

    if (_klctool_check_name(name) < 0) {
        fprintf(stderr, "function name %s invalid \n", name);
        return -1;
    }

    func = _klctool_idname_func_jit(data, data_len, attr, module_name, name);
    if (! func) {
        return -1;
    }

    func->type = type;
    func->id = id;
    func->hdr.replace = replace;

    int ret = _klctool_send_nl_msg(nl, nlcmd, func, func->hdr.size, NULL);

    MEM_Free(func);

    return ret;
}

static int _klctool_load_id_func_one(KLCTOOL_LOAD_INFO_S *loadinfo, int type, int id, char *name)
{
    ELF_SECTION_S *sec = loadinfo->sec;
    KLC_FUNC_ATTR_S * attr;

    attr = _klctool_get_func_attr(loadinfo->elf, name);

    return _klctool_add_func(loadinfo->nl, sec->data->d_buf, sec->data->d_size, attr,
            loadinfo->module->name, type, id, name, loadinfo->replace);
}

static int _klctool_load_name_func_one(KLCTOOL_LOAD_INFO_S *loadinfo, int type, char *name)
{
    ELF_SECTION_S *sec = loadinfo->sec;
    KLC_FUNC_ATTR_S * attr;

    attr = _klctool_get_func_attr(loadinfo->elf, name);

    return _klctool_add_func(loadinfo->nl, sec->data->d_buf, sec->data->d_size, attr,
            loadinfo->module->name, type, -1, name, loadinfo->replace);
}

static int _klctool_check_func(KLCTOOL_LOAD_INFO_S *loadinfo)
{
    ELF_SECTION_S *sec = loadinfo->sec;
    void *data = sec->data->d_buf;
    int data_len = sec->data->d_size;
    char msg[512];
    int ret;

    ret = BPF_Check(data, data_len / 8, msg, sizeof(msg));
    if (ret < 0) {
        BS_PRINT_ERR("%s: %s \n", loadinfo->sec->shname, msg);
    }

    return ret;
}

static int _klctool_check_id_func(KLCTOOL_LOAD_INFO_S *loadinfo)
{
    if (loadinfo->tok_num < 4) {
        fprintf(stderr, "Function %s is invalid \n", loadinfo->sec->shname);
        return -1;
    }

    char *id_str = loadinfo->tok[3];
    int id = TXT_Str2Ui(id_str);
    if ((id < 0) || (id >= KLC_FUNC_ID_MAX)) {
        fprintf(stderr, "Function %s is invalid \n", loadinfo->sec->shname);
        return -1;
    }

    return _klctool_check_func(loadinfo);
}

static int _klctool_check_name_func(KLCTOOL_LOAD_INFO_S *loadinfo)
{
    if (loadinfo->tok_num < 3) {
        fprintf(stderr, "Function %s is invalid \n", loadinfo->sec->shname);
        return -1;
    }

    return _klctool_check_func(loadinfo);
}

#ifdef USE_KLCTOOL_MAPFUNC
static int _klctool_open_hashmap_fd(KLCTOOL_LOAD_INFO_S *loadinfo)
{
    ELF_SECTION_S *sec = loadinfo->sec;
    char *name = loadinfo->tok[2];
    char *map_name = loadinfo->tok[3];
    char path[128];
    int fd;

    if (_klctool_check_name(name) < 0) {
        fprintf(stderr, "function name %s invalid \n", name);
        return -1;
    }
    if (loadinfo->tok_num < 4) {
        fprintf(stderr, "Function %s is invalid \n", sec->shname);
        return -1;
    }
    if (sec->data->d_size > (KLC_MAPFUNC_INSN_SIZE - sizeof(g_klctool_insn_hdr))) {
        fprintf(stderr, "Function %s is too large \n", sec->shname);
        return -1;
    }
    int nlen = SNPRINTF(path, sizeof(path), "/sys/fs/bpf/tc/globals/%s", map_name);
    if (nlen < 0) {
        fprintf(stderr, "Function %s is invalid \n", sec->shname);
        return -1;
    }
    fd = bpf_obj_get(path);
    if (fd < 0) {
        fprintf(stderr, "bpf_obj_get %s failed.\n", path);
        return -1;
    }
    return fd;
}
#endif

static int _klctool_check_map_func(KLCTOOL_LOAD_INFO_S *loadinfo)
{
    char key[KLC_FUNC_NAME_SIZE];
    char *name = loadinfo->tok[2];

    int nlen = SNPRINTF(key, sizeof(key), "%s.%s", loadinfo->module->name, name);
    if (nlen < 0) {
        BS_PRINT_ERR("%s.%s too long\n", loadinfo->module->name, name);
        return -1;
    }

#ifdef USE_KLCTOOL_MAPFUNC
    int fd = _klctool_open_hashmap_fd(loadinfo);
    if (fd < 0) {
        BS_PRINT_ERR("map function %s open failed \n", name);
        return -1;
    }
    close(fd);
#endif

    return _klctool_check_func(loadinfo);
}


/* klc/idfunc/func_name/func_id */
static int _klctool_load_id_func(KLCTOOL_LOAD_INFO_S *loadinfo)
{
    int type = KLC_FUNC_TYPE_ID;
    char *name = loadinfo->tok[2];
    char *id_str = loadinfo->tok[3];

    if (_klctool_check_id_func(loadinfo) < 0) {
        return -1;
    }
    
    int id = TXT_Str2Ui(id_str);
    if ((id < 0) || (id >= KLC_FUNC_ID_MAX)) {
        return -1;
    }

    return _klctool_load_id_func_one(loadinfo, type, id, name);
}

static int _klctool_load_name_func(KLCTOOL_LOAD_INFO_S *loadinfo)
{
    int type = KLC_FUNC_TYPE_NAME;

    if (_klctool_check_name_func(loadinfo) < 0) {
        return -1;
    }

    return _klctool_load_name_func_one(loadinfo, type, loadinfo->tok[2]);
}

static int _klctool_load_evob_one(KLCTOOL_LOAD_INFO_S *loadinfo, int cmd, KLC_EVENT_OB_S *ob)
{
    int ret;
    KLC_EVENT_OB_S *event_ob = NULL;
    int len = -1;

    if (_klctool_check_name(ob->hdr.name) < 0) {
        fprintf(stderr, "evob name %s invalid \n", ob->hdr.name);
        return -1;
    }

    len = sizeof(KLC_EVENT_OB_S) + ob->private_data_size;

    event_ob = MEM_ZMalloc(len);
    if (! event_ob) {
        fprintf(stderr, "Can't alloc memory for evob %s \n", ob->hdr.name);
        goto Exit;
    }

    memcpy(event_ob, ob, sizeof(KLC_EVENT_OB_S));
    memset(&event_ob->hdr, 0, sizeof(event_ob->hdr));
    event_ob->hdr.replace = loadinfo->replace;
    event_ob->hdr.size = len;
    int nlen = SNPRINTF(event_ob->hdr.name, sizeof(event_ob->hdr.name), "%s.%s", loadinfo->module->name, ob->hdr.name);
    if (nlen < 0) {
        fprintf(stderr, "%s.%s too long\n", loadinfo->module->name, ob->hdr.name);
        goto Exit;
    }

    if (strchr(event_ob->func, '.') == NULL) {
        /* 没有给定module名,使用自身的名字 */
        nlen = SNPRINTF(event_ob->func, sizeof(event_ob->func), "%s.%s", loadinfo->module->name, ob->func);
        if (nlen < 0) {
            fprintf(stderr, "%s.%s too long\n", loadinfo->module->name, ob->hdr.name);
            goto Exit;
        }
    }

    ret = _klctool_common_cmd(loadinfo->nl, cmd, event_ob, len);
    if (ret < 0) {
        fprintf(stderr, "evob %s error: %d \n", ob->hdr.name, ret);
    }

Exit:
    if (event_ob) {
        MEM_Free(event_ob);
    }
    return ret;
}

/* klc/evob */
static int _klctool_load_evob(KLCTOOL_LOAD_INFO_S *loadinfo)
{
    ELF_SECTION_S *sec = loadinfo->sec;
    KLC_EVENT_OB_S *ob = sec->data->d_buf;
    int data_len = sec->data->d_size;
    int count = data_len / sizeof(KLC_EVENT_OB_S);
    int i;
    int nlcmd = KLC_NL_TYPE_CMD(KLC_NL_TYPE_EVENT, KLC_NL_EVOB_ADD);

    for (i=0; i<count; i++, ob++) {
        _klctool_load_evob_one(loadinfo, nlcmd, ob);
    }

    return 0;
}

static int _klctool_unload_map(NETLINK_S *nl, int cmd, void *ud, int nouse)
{
    USER_HANDLE_S *uh = ud;
    char *module_name = uh->ahUserHandle[0];
    char *map_name = uh->ahUserHandle[1];
    KLCTOOL_MAP_S klcmap = {0};
    int nlcmd = KLC_NL_TYPE_CMD(KLC_NL_TYPE_MAP, KLC_NL_MAP_DEL);

    int nlen = SNPRINTF(klcmap.hdr.name, sizeof(klcmap.hdr.name), "%s.%s", module_name, map_name);
    if (nlen < 0) {
        fprintf(stderr, "%s.%s map name too long \n", module_name, map_name);
        return -1;
    }

    klcmap.hdr.size = sizeof(klcmap);

    return _klctool_common_cmd(nl, nlcmd, &klcmap, sizeof(klcmap));
}

static int _klctool_load_map_by_fd(void *nl, int fd, char *module_name, char *name, int replace)
{
    int nlcmd = KLC_NL_TYPE_CMD(KLC_NL_TYPE_MAP, KLC_NL_MAP_ADD);
    KLCTOOL_MAP_S klcmap = {0};

    if (_klctool_check_name(name) < 0) {
        fprintf(stderr, "module %s map name %s invalid \n", module_name, name);
        return -1;
    }

    klcmap.hdr.size = sizeof(klcmap);
    klcmap.hdr.replace = replace;
    klcmap.fd = fd;

    int nlen = SNPRINTF(klcmap.hdr.name, sizeof(klcmap.hdr.name), "%s.%s", module_name, name);
    if (nlen < 0) {
        fprintf(stderr, "%s.%s map name too long \n", module_name, name);
        return -1;
    }

    int ret = _klctool_common_cmd(nl, nlcmd, &klcmap, sizeof(klcmap));
    if (ret < 0) {
        if (ret < KO_ERR_MAX) {
            fprintf(stderr, "Can't load map %s, error:%s \n", klcmap.hdr.name, _klctool_get_ko_err_msg(ret));
        } else {
            fprintf(stderr, "bind %s map error: %d \n", klcmap.hdr.name, ret);
        }
    }

    close(fd);

    return 0;
}

static int _klctool_load_map_by_fd_cb(NETLINK_S *nl, int cmd, void *ud, int nouse)
{
    USER_HANDLE_S *uh = ud;

    int fd = HANDLE_UINT(uh->ahUserHandle[0]);
    char *module_name = uh->ahUserHandle[1];
    char *map_name = uh->ahUserHandle[2];

    return _klctool_load_map_by_fd(nl, fd, module_name, map_name, 0);
}

static int _klctool_load_map_one(KLCTOOL_LOAD_INFO_S *loadinfo, KLC_MAP_S *map)
{
    BPF_MAP_PARAM_S p = {0};
    int fd;
    int ret;

    if (_klctool_check_name(map->name) < 0) {
        fprintf(stderr, "module %s map name %s invalid \n", loadinfo->module->name, map->name);
        return -1;
    }

    if (strlen(loadinfo->module->name) + strlen(map->name) + 1 >= KUTL_KNODE_NAME_SIZE) {
        fprintf(stderr, "%s.%s map name too long \n", loadinfo->module->name, map->name);
        return -1;
    }

    char *pin_name = map->name;
    if (map->pin_name) {
        pin_name = map->pin_name;
    } 

    char *pin_path = "/sys/fs/bpf/klc/globals";
    if (loadinfo->map_pin_path) {
        pin_path = loadinfo->map_pin_path;
    }

    ret = SNPRINTF(p.filename, sizeof(p.filename), "%s/%s", pin_path, pin_name);
    if (ret < 0) {
        fprintf(stderr, "%s:%s map name too long \n", loadinfo->module->name, map->name);
        return -1;
    }

    p.type = map->type;
    p.key_size = map->key_size;
    p.value_size = map->value_size;
    p.max_elem = map->max_entries;
    p.flags = map->map_flags;
    p.pinning = map->pinning;

    fd = BPF_CreateMap(&p);
    if (fd < 0) {
        fprintf(stderr, "create map %s:%s failed, err:%d \n", loadinfo->module->name, map->name, fd);
        return -1;
    }

    return _klctool_load_map_by_fd(loadinfo->nl, fd, loadinfo->module->name, map->name, loadinfo->replace);
}

static int _klctool_load_map(KLCTOOL_LOAD_INFO_S *loadinfo)
{
    ELF_SECTION_S *sec = loadinfo->sec;
    KLC_MAP_S *map = sec->data->d_buf;
    int data_len = sec->data->d_size;
    int count = data_len / sizeof(KLC_MAP_S);
    int i;

    for (i=0; i<count; i++, map++) {
        _klctool_load_map_one(loadinfo, map);
    }

    return 0;
}

static int _klctool_load_init_one(KLCTOOL_LOAD_INFO_S *loadinfo, int cmd, KLC_FUNC_INIT_S *init)
{
    KLC_FUNC_INIT_S ele;

    memcpy(&ele, init, sizeof(ele));

    if (strchr(init->func, '.') == NULL) {
        int nlen = SNPRINTF(ele.func, sizeof(ele.func), "%s.%s", loadinfo->module->name, init->func);
        if (nlen < 0) {
            fprintf(stderr, "%s.%s too long\n", loadinfo->module->name, init->func);
            return -1;
        }
    }

    return _klctool_common_cmd(loadinfo->nl, cmd, &ele, sizeof(KLC_FUNC_INIT_S));
}

/* klc/init */
static int _klctool_load_init(KLCTOOL_LOAD_INFO_S *loadinfo)
{
    ELF_SECTION_S *sec = loadinfo->sec;
    KLC_FUNC_INIT_S *init = sec->data->d_buf;
    int data_len = sec->data->d_size;
    int count = data_len / sizeof(KLC_FUNC_INIT_S);
    int i;
    int ret;

    int nlcmd = KLC_NL_TYPE_CMD(KLC_NL_TYPE_NAME_FUNC, KLC_NL_FUNC_INIT);

    for (i=0; i<count; i++, init++) {
        ret = _klctool_load_init_one(loadinfo, nlcmd, init);
        if (ret < 0) {
            break;
        }
    }

    return ret;
}

/* klc/mapfunc/func_name/map_name */
static int _klctool_load_hashmap_func(KLCTOOL_LOAD_INFO_S *loadinfo)
{
#ifdef USE_KLCTOOL_MAPFUNC
    ELF_SECTION_S *sec = loadinfo->sec;
    char *name = loadinfo->tok[2];
    char key[KLC_FUNC_NAME_SIZE];
    UINT64 flag = BPF_NOEXIST;
    void *data = sec->data->d_buf;
    int data_len = sec->data->d_size;

    if (_klctool_check_map_func(loadinfo) < 0) {
        return -1;
    }

    if (loadinfo->replace) {
        flag = BPF_ANY;
    }

    memset(key, 0, sizeof(key));

    int nlen = SNPRINTF(key, sizeof(key), "%s.%s", loadinfo->module->name, name);
    if (nlen < 0) {
        return -1;
    }

    int fd = _klctool_open_hashmap_fd(loadinfo);
    if (fd < 0) {
        BS_PRINT_ERR("map %s open failed \n", name);
        return -1;
    }

    if (loadinfo->cmd == KLC_NL_OBJ_LOAD) {
        char *code = MEM_ZMalloc(KLC_MAPFUNC_INSN_SIZE);
        if (code) {
            memcpy(code, &g_klctool_insn_hdr, sizeof(g_klctool_insn_hdr));
            memcpy(&code[sizeof(g_klctool_insn_hdr)], data, data_len);
            bpf_map_update_elem(fd, key, code, flag);
            MEM_Free(code);
        }
    } else {
        bpf_map_delete_elem(fd, key);
    }

    close(fd);
#endif

    return 0;
}

static KLC_MODULE_INFO_S * _klctool_get_module(ELF_S *elf)
{
    ELF_SECTION_S sec;

    if (_klctool_get_sec(elf, KLCTOOL_SEC_MODULE, &sec) < 0) {
        return NULL;
    }

    return sec.data->d_buf;
}

static int _klctool_load_module(void *nl, char *module_name, int data_size, int replace)
{
    KLC_MODULE_S *mod;
    int ret = -1;
    int nlcmd;
    int len = -1;

    len = sizeof(KLC_MODULE_S) + data_size;

    mod = MEM_ZMalloc(len);
    if (! mod) {
        fprintf(stderr, "Can't alloc memory for module %s \n", module_name);
        return -1;
    }

    int nlen = SNPRINTF(mod->hdr.name, sizeof(mod->hdr.name), "%s", module_name);
    if (nlen < 0) {
        fprintf(stderr, "%s too long\n", module_name);
        MEM_Free(mod);
        return -1;
    }

    mod->hdr.size = len;
    mod->hdr.replace = replace;
    mod->data_size = data_size;

    nlcmd = KLC_NL_TYPE_CMD(KLC_NL_TYPE_MODULE, KLC_NL_MODULE_ADD);

    ret = _klctool_send_nl_msg(nl, nlcmd, mod, len, NULL);
    if (ret < KO_ERR_MAX) {
        fprintf(stderr, "Can't load Module %s, error:%s \n", module_name, _klctool_get_ko_err_msg(ret));
    }

    MEM_Free(mod);

    return ret;
}

static int _klctool_load_module_cb(NETLINK_S *nl, int cmd, void *data, int data_len)
{
    USER_HANDLE_S *uh = data;
    char *module_name = uh->ahUserHandle[0];
    int data_size = HANDLE_UINT(uh->ahUserHandle[1]);
    int replace = HANDLE_UINT(uh->ahUserHandle[2]);

    return _klctool_load_module(nl, module_name, data_size, replace);
}

static int _klctool_unload_module(void *nl, char *module_name)
{
    KLC_MODULE_S mod = {0};

    int nlen = SNPRINTF(mod.hdr.name, sizeof(mod.hdr.name), "%s", module_name);
    if (nlen < 0) {
        fprintf(stderr, "%s too long\n", module_name);
        return -1;
    }

    mod.hdr.size = sizeof(mod);

    int nlcmd = KLC_NL_TYPE_CMD(KLC_NL_TYPE_MODULE, KLC_NL_MODULE_DEL);

    _klctool_send_nl_msg(nl, nlcmd, &mod, sizeof(mod), NULL);

    return 0;
}

static int _klctool_process_module(KLCTOOL_LOAD_INFO_S *loadinfo)
{
    if (loadinfo->cmd == KLC_NL_OBJ_LOAD) {
        return _klctool_load_module(loadinfo->nl, loadinfo->module->name,
                loadinfo->module->data_size, loadinfo->replace);
    } 

    return _klctool_unload_module(loadinfo->nl, loadinfo->module->name);
}

static int _klctool_unload_subs(KLCTOOL_LOAD_INFO_S *loadinfo)
{
    ELF_SECTION_S sec;
    void *iter = NULL;
    char buf[256];
    char *tok[16];
    int tok_num;
    ELF_S *elf = loadinfo->elf;

    while ((iter = ELF_GetNextSection(elf, iter, &sec))) {

        strlcpy(buf, sec.shname, sizeof(buf));

        tok_num = TXT_StrToToken(buf, "/", tok, 16);
        if (tok_num < 2) {
            continue;
        }

		if (strcmp(tok[0], KLCTOOL_SEC_PREFIX) != 0) {
            continue;
        }

        loadinfo->tok = tok;
        loadinfo->tok_num = tok_num;
        loadinfo->sec = &sec;

        if (strcmp(tok[1], "mapfunc") == 0) {
            _klctool_load_hashmap_func(loadinfo);
        }
    }

    return 0;
}

/* 检查各个项目是否正确 */
static int _klctool_check_subs(KLCTOOL_LOAD_INFO_S *loadinfo)
{
    void *iter = NULL;
    ELF_SECTION_S sec;
    char buf[256];
    char *tok[16];
    int tok_num;
    int ret = 0;
    ELF_S *elf = loadinfo->elf;

    while ((iter = ELF_GetNextSection(elf, iter, &sec))) {
        strlcpy(buf, sec.shname, sizeof(buf));
        tok_num = TXT_StrToToken(buf, "/", tok, 16);
        if (tok_num < 2) {
            continue;
        }
		if (strcmp(tok[0], KLCTOOL_SEC_PREFIX) != 0) {
            continue;
        }

        loadinfo->tok = tok;
        loadinfo->tok_num = tok_num;
        loadinfo->sec = &sec;

        if (strcmp(tok[1], "idfunc") == 0) {
            ret |= _klctool_check_id_func(loadinfo);
        } else if (strcmp(tok[1], "namefunc") == 0) {
            ret |= _klctool_check_name_func(loadinfo);
        } else if (strcmp(tok[1], "mapfunc") == 0) {
            ret |= _klctool_check_map_func(loadinfo);
        } else {
            continue;
        }
    }

    return ret;
}

static int _klctool_load_subs(KLCTOOL_LOAD_INFO_S *loadinfo)
{
    ELF_S *elf = loadinfo->elf;
    void *iter = NULL;
    ELF_SECTION_S sec;
    char buf[256];
    char *tok[16];
    int tok_num;

    _klctool_get_os_helper(loadinfo->nl, &g_klctool_oshelpers);

    while ((iter = ELF_GetNextSection(elf, iter, &sec))) {

        strlcpy(buf, sec.shname, sizeof(buf));

        tok_num = TXT_StrToToken(buf, "/", tok, 16);
        if (tok_num < 2) {
            continue;
        }

		if (strcmp(tok[0], KLCTOOL_SEC_PREFIX) != 0) {
            continue;
        }

        loadinfo->tok = tok;
        loadinfo->tok_num = tok_num;
        loadinfo->sec = &sec;

        if (strcmp(tok[1], "idfunc") == 0) {
            _klctool_load_id_func(loadinfo);
        } else if (strcmp(tok[1], "namefunc") == 0) {
            _klctool_load_name_func(loadinfo);
        } else if (strcmp(tok[1], "mapfunc") == 0) {
            _klctool_load_hashmap_func(loadinfo);
        } else if (strcmp(tok[1], "evob") == 0) {
            _klctool_load_evob(loadinfo);
        } else if (strcmp(tok[1], "init") == 0) {
            _klctool_load_init(loadinfo);
        } else if (strcmp(tok[1], "maps") == 0) {
            _klctool_load_map(loadinfo);
        } else if (strcmp(tok[1], "module") == 0) {
            continue;
        } else if (strcmp(tok[1], "mapp_desc") == 0) {
            continue;
        } else if (strcmp(tok[1], "func_attr") == 0) {
            continue;
        } else {
            fprintf(stderr, "Ignore %s \n", sec.shname);
            continue;
        }
    }

    return 0;
}

/*
 *   section格式: klc/type/func_name/...
 *     ID类型: klc/idfunc/func_name/func_id
 *         eg: klc/idfunc/test_func/0
 *     Name类型: klc/namefunc/func_name
 *         eg: klc/namefunc/test_func
 *     Hash Map类型: klc/mapfunc/func_name/map_name
 *         eg: klc/mapfunc/test_func/my_func_map
 *     Evob类型: klc/evob
 *     Init类型: klc/init
 *     Map类型:  klc/maps
 */
static int _klctool_load_by_file(NETLINK_S *nl, int cmd, KLCTOOL_LOAD_S *load, ELF_S *elf)
{
    KLC_MODULE_INFO_S *module;
    KLCTOOL_LOAD_INFO_S loadinfo = {0};

    module = _klctool_get_module(elf);
    if (! module) {
        fprintf(stderr, "Can't get module name from file %s \n", load->file);
        return -1;
    }

    if (load->module_name) {
        strlcpy(module->name, load->module_name, sizeof(module->name));
    }

    if (_klctool_check_name(module->name) < 0) {
        fprintf(stderr, "Module name %s of file %s invalid \n", module->name, load->file);
        return -1;
    }

    loadinfo.module = module;
    loadinfo.cmd = cmd;
    loadinfo.nl = nl;
    loadinfo.elf = elf;
    loadinfo.file = load->file;
    loadinfo.map_pin_path = load->map_pin_path;
    loadinfo.replace = load->replace;

    if (_klctool_check_subs(&loadinfo) < 0) {
        return -1;
    }

    if (_klctool_process_module(&loadinfo) < 0) {
        return -1;
    }

    if (cmd == KLC_NL_OBJ_UNLOAD) {
        return _klctool_unload_subs(&loadinfo);
    }

    return _klctool_load_subs(&loadinfo);
}

static int _klctool_load_by_filename(NETLINK_S *nl, int cmd, KLCTOOL_LOAD_S *load)
{
    int ret;
    ELF_S elf;

    ret = ELF_Open(load->file, &elf);
    if (ret < 0) {
        fprintf(stderr, "Can't open %s \n", load->file);
        return -1;
    }

    ret = _klctool_load_by_file(nl, cmd, load, &elf);

    ELF_Close(&elf);

    return ret;
}

static int _klctool_file_load(NETLINK_S *nl, int cmd, void *ud, int nouse)
{
    KLCTOOL_LOAD_S *load = ud;

    return _klctool_load_by_filename(nl, cmd, load);
}

static void _klctool_func_show_info(KUTL_NL_GET_S *show)
{
    int i;
    KLC_FUNC_S *func = (void*)(show + 1);

    for (i=0; i<show->count; i++) {
        if (func->type == KLC_FUNC_TYPE_ID) {
            printf(" name:%s, id:%u, jitted:%d, len:%u \n",
                    func->hdr.name, func->id, func->hdr.exe, func->insn_len);
        } else {
            printf(" name:%s, jitted:%d, len:%u \n", func->hdr.name, func->hdr.exe, func->insn_len);
        }
        func ++;
    }
}

static int _klctool_func_show_many(NETLINK_S *nl, int cmd, void *data, int data_len)
{
    KUTL_NL_GET_S *show;
    int ret;

    show = MEM_Malloc(sizeof(KUTL_NL_GET_S) + KLCTOOL_SHOW_MAX * sizeof(KLC_FUNC_S));
    if (! show) {
		fprintf(stderr, "Can't alloc memory \n");
        return -1;
    }

    memset(show, 0, sizeof(KUTL_NL_GET_S));

    show->max_count = KLCTOOL_SHOW_MAX;
    show->ele_size = sizeof(KLC_FUNC_S);
    show->from = 0;

    do {
        ret = _klctool_send_nl_msg(nl, cmd, data, data_len, show);
        if (ret < 0) {
            break;
        }

        _klctool_func_show_info(show);
        show->from = show->pos + 1;
    } while(show->count > 0);

    MEM_Free(show);

    return ret;
}

static int _klctool_func_show_one(NETLINK_S *nl, int cmd, void *data, int data_len)
{
    KUTL_NL_GET_S *show;
    int ret;

    show = MEM_Malloc(sizeof(KUTL_NL_GET_S) + sizeof(KLC_FUNC_S));
    if (! show) {
		fprintf(stderr, "Can't alloc memory \n");
        return -1;
    }

    memset(show, 0, sizeof(KUTL_NL_GET_S));

    show->max_count = 1;
    show->ele_size = sizeof(KLC_FUNC_S);

    ret = _klctool_send_nl_msg(nl, cmd, data, data_len, show);
    if (ret >= 0) {
        _klctool_func_show_info(show);
    }

    MEM_Free(show);

    return ret;
}

static void _klctool_func_dump_info(KLC_FUNC_DUMP_S *dump)
{
    int count;
    int i;
    unsigned char *p;

    if (dump->write_size <= 0) {
        return;
    }

    count = dump->write_size / 8;
    p = dump->insn;

    for (i=0; i<count; i++) {
        printf("%02x %02x %02x %02x %02x %02x %02x %02x\n", 
                p[i], p[i+1], p[i+2], p[i+3], p[i+4], p[i+5], p[i+6], p[i+7]);
    }
}

static int _klctool_func_dump(NETLINK_S *nl, int cmd, void *data, int data_len)
{
    KLC_FUNC_DUMP_S *dump;
    int ret;

    dump = MEM_Malloc(sizeof(KLC_FUNC_DUMP_S) + KLCTOOL_DUMP_SIZE);
    if (! dump) {
		fprintf(stderr, "Can't alloc memory \n");
        return -1;
    }

    memset(dump, 0, sizeof(KLC_FUNC_DUMP_S));

    dump->dump_size = KLCTOOL_DUMP_SIZE;

    ret = _klctool_send_nl_msg(nl, cmd, data, data_len, dump);
    if (ret >= 0) {
        _klctool_func_dump_info(dump);
    }

    MEM_Free(dump);

    return ret;
}

static void _klctool_evob_show_info(KUTL_NL_GET_S *show)
{
    int i;
    KLC_EVENT_OB_S *ob = (void*)(show + 1);

    for (i=0; i<show->count; i++) {
        printf(" event:%d, sub_event:%llu, name:%s, function:%s\n",
                ob->event, ob->sub_event, ob->hdr.name, ob->func);
        ob ++;
    }
}

static void _klctool_module_show_info(KUTL_NL_GET_S *show)
{
    int i;
    KLC_MODULE_S *mod = (void*)(show + 1);

    for (i=0; i<show->count; i++) {
        printf(" name:%s\n",mod->hdr.name);
        mod ++;
    }
}

static void _klctool_map_show_info(KUTL_NL_GET_S *show)
{
    int i;
    KLCTOOL_MAP_S *map = (void*)(show + 1);

    for (i=0; i<show->count; i++) {
        printf(" name: %s\n", map->hdr.name);
        map ++;
    }
}

static int _klctool_evob_show(NETLINK_S *nl, int nouse, void *data, int data_len)
{
    KUTL_NL_GET_S *show;
    int ret;
    int cmd = KLC_NL_TYPE_CMD(KLC_NL_TYPE_EVENT, KLC_NL_EVOB_GET);

    show = MEM_Malloc(sizeof(KUTL_NL_GET_S) + KLCTOOL_SHOW_MAX * sizeof(KLC_EVENT_OB_S));
    if (! show) {
		fprintf(stderr, "Can't alloc memory \n");
        return -1;
    }
    memset(show, 0, sizeof(KUTL_NL_GET_S));

    show->max_count = KLCTOOL_SHOW_MAX;
    show->ele_size = sizeof(KLC_EVENT_OB_S);
    show->from = 0;

    do {
        ret = _klctool_send_nl_msg(nl, cmd, data, data_len, show);
        if (ret < 0) {
            break;
        }

        _klctool_evob_show_info(show);
        show->from = show->pos + 1;
    } while(show->count > 0);

    MEM_Free(show);

    return ret;
}

static int _klctool_oshelper_show(NETLINK_S *nl, int nouse, void *data, int data_len)
{
    KLC_OSHELPER_LIST_S list = {0};
    int ret;
    int i;

    ret = _klctool_get_os_helper(nl, &list);
    if (ret < 0) {
        return ret;
    }

    for (i=0; i<KLC_OSHELPER_MAX; i++) {
        if (list.helpers[i]) {
            printf(" %d: %p \n", i, list.helpers[i]);
        }
    }

    return 0;
}

static int _klctool_module_show(NETLINK_S *nl, int cmd, void *data, int data_len)
{
    KUTL_NL_GET_S *show;
    int ret;

    show = MEM_Malloc(sizeof(KUTL_NL_GET_S) + KLCTOOL_SHOW_MAX * sizeof(KLC_MODULE_S));
    if (! show) {
		fprintf(stderr, "Can't alloc memory \n");
        return -1;
    }
    memset(show, 0, sizeof(KUTL_NL_GET_S));

    show->max_count = KLCTOOL_SHOW_MAX;
    show->ele_size = sizeof(KLC_MODULE_S);
    show->from = 0;

    do {
        ret = _klctool_send_nl_msg(nl, cmd, data, data_len, show);
        if (ret < 0) {
            break;
        }

        _klctool_module_show_info(show);
        show->from = show->pos + 1;
    } while(show->count > 0);

    MEM_Free(show);

    return ret;
}

static int _klctool_map_show(NETLINK_S *nl, int nouse, void *data, int data_len)
{
    KUTL_NL_GET_S *show;
    int ret;
    int cmd = KLC_NL_TYPE_CMD(KLC_NL_TYPE_MAP, KLC_NL_MAP_GET);

    show = MEM_Malloc(sizeof(KUTL_NL_GET_S) + KLCTOOL_SHOW_MAX * sizeof(KLCTOOL_MAP_S));
    if (! show) {
		fprintf(stderr, "Can't alloc memory \n");
        return -1;
    }
    memset(show, 0, sizeof(KUTL_NL_GET_S));

    show->max_count = KLCTOOL_SHOW_MAX;
    show->ele_size = sizeof(KLCTOOL_MAP_S);
    show->from = 0;

    do {
        ret = _klctool_send_nl_msg(nl, cmd, data, data_len, show);
        if (ret < 0) {
            break;
        }

        _klctool_map_show_info(show);
        show->from = show->pos + 1;
    } while(show->count > 0);

    MEM_Free(show);

    return ret;
}

static int _klctool_func_show_idfunc(int id)
{
    KLC_FUNC_S func = {0};
    int ret;
    int cmd = KLC_NL_TYPE_CMD(KLC_NL_TYPE_ID_FUNC, KLC_NL_FUNC_GET);

    func.id = id;
    func.type = KLC_FUNC_TYPE_ID;

    if (func.id < 0) {
        ret = NetLink_Do(_klctool_func_show_many, cmd, &func, sizeof(func));
    } else {
        ret = NetLink_Do(_klctool_func_show_one, cmd, &func, sizeof(func));
    }

    return ret;
}

static int _klctool_func_show_namefunc(char *key)
{
    KLC_FUNC_S func = {0};
    int ret;
    int cmd = KLC_NL_TYPE_CMD(KLC_NL_TYPE_NAME_FUNC, KLC_NL_FUNC_GET);

    func.id = -1;
    func.type = KLC_FUNC_TYPE_NAME;

    if (key) {
        strlcpy(func.hdr.name, key, sizeof(func.hdr.name));
    }

    if (! func.hdr.name[0]) {
        ret = NetLink_Do(_klctool_func_show_many, cmd, &func, sizeof(func));
    } else {
        ret = NetLink_Do(_klctool_func_show_one, cmd, &func, sizeof(func));
    }

    return ret;
}

int KLCTOOL_LoadFile(char *filename, int replace, char *module_name, char *map_pin_path)
{
    KLCTOOL_LOAD_S load = {0};

    if ((module_name) && (strlen(module_name) >= KUTL_KNODE_NAME_SIZE)) {
        BS_DBGASSERT(0);
        return -1;
    }

    load.file = filename;
    load.module_name = module_name;
    load.map_pin_path = map_pin_path;
    load.replace = replace;

    return NetLink_Do(_klctool_file_load, KLC_NL_OBJ_LOAD, &load, 0);
}

int KLCTOOL_UnLoadFile(char *filename)
{
    KLCTOOL_LOAD_S load = {0};

    load.file = filename;

    return NetLink_Do(_klctool_file_load, KLC_NL_OBJ_UNLOAD, &load, 0);
}

int KLCTOOL_LoadMapByFd(int fd, char *module_name, char *map_name)
{
    USER_HANDLE_S uh;

    uh.ahUserHandle[0] = UINT_HANDLE(fd);
    uh.ahUserHandle[1] = module_name;
    uh.ahUserHandle[2] = map_name;

    return NetLink_Do(_klctool_load_map_by_fd_cb, 0, &uh, 0);
}

int KLCTOOL_UnloadMap(char *module_name, char *map_name)
{
    USER_HANDLE_S uh;

    uh.ahUserHandle[0] = module_name;
    uh.ahUserHandle[1] = map_name;

    return NetLink_Do(_klctool_unload_map, 0, &uh, 0);
}

int KLCTOOL_LoadModule(char *module_name, int data_size, int replace)
{
    USER_HANDLE_S uh;

    uh.ahUserHandle[0] = module_name;
    uh.ahUserHandle[1] = UINT_HANDLE(data_size);
    uh.ahUserHandle[2] = UINT_HANDLE(replace);

    return NetLink_Do(_klctool_load_module_cb, 0, &uh, 0);
}

int KLCTOOL_DelModule(char *module_name)
{
    KLC_MODULE_S mod = {0};
    int cmd = KLC_NL_TYPE_CMD(KLC_NL_TYPE_MODULE, KLC_NL_MODULE_DEL);

    if (module_name) {
        strlcpy(mod.hdr.name, module_name, sizeof(mod.hdr.name));
    }

    return NetLink_Do(_klctool_common_cmd, cmd, &mod, sizeof(mod));
}

int KLCTOOL_DelIDFunc(int id)
{
    KLC_FUNC_S func = {0};
    int cmd = KLC_NL_TYPE_CMD(KLC_NL_TYPE_ID_FUNC, KLC_NL_FUNC_DEL);

    func.type = KLC_FUNC_TYPE_ID;
    func.id = id;

    return NetLink_Do(_klctool_common_cmd, cmd, &func, sizeof(func));
}

int KLCTOOL_DelNameFunc(char *name)
{
    KLC_FUNC_S func = {0};
    int cmd = KLC_NL_TYPE_CMD(KLC_NL_TYPE_NAME_FUNC, KLC_NL_FUNC_DEL);

    func.type = KLC_FUNC_TYPE_NAME;
    func.id = -1;
    if (name) {
        strlcpy(func.hdr.name, name, sizeof(func.hdr.name));
    }

    return NetLink_Do(_klctool_common_cmd, cmd, &func, sizeof(func));
}

int KLCTOOL_DelEvob(int event, char *name)
{
    KLC_EVENT_OB_S ob = {0};
    int cmd = KLC_NL_TYPE_CMD(KLC_NL_TYPE_EVENT, KLC_NL_EVOB_DEL);

    if ((event < 0) || (! name)) {
        RETURN(BS_ERR);
    }

    ob.event = event;
    strlcpy(ob.hdr.name, name, sizeof(ob.hdr.name));

    return NetLink_Do(_klctool_common_cmd, cmd, &ob, sizeof(ob));
}

int KLCTOOL_ShowModule()
{
    int cmd = KLC_NL_TYPE_CMD(KLC_NL_TYPE_MODULE, KLC_NL_MODULE_GET);
    return NetLink_Do(_klctool_module_show, cmd, NULL, 0);
}

int KLCTOOL_ShowIDFunc(int id)
{
    return _klctool_func_show_idfunc(id);
}

int KLCTOOL_ShowNameFunc(char *name)
{
    return _klctool_func_show_namefunc(name);
}

int KLCTOOL_ShowNameMap()
{
    return NetLink_Do(_klctool_map_show, 0, NULL, 0);
}

int KLCTOOL_ShowEvob()
{
    return NetLink_Do(_klctool_evob_show, 0, NULL, 0);
}

int KLCTOOL_ShowOsHelper()
{
    return NetLink_Do(_klctool_oshelper_show, 0, NULL, 0);
}

int KLCTOOL_DumpIDFunc(int id)
{
    KLC_FUNC_S func = {0};
    int cmd = KLC_NL_TYPE_CMD(KLC_NL_TYPE_ID_FUNC, KLC_NL_FUNC_DUMP);

    func.type = KLC_FUNC_TYPE_ID;
    func.id = id;

    return NetLink_Do(_klctool_func_dump, cmd, &func, sizeof(func));
}

int KLCTOOL_DumpNameFunc(char *name)
{
    KLC_FUNC_S func = {0};
    int cmd = KLC_NL_TYPE_CMD(KLC_NL_TYPE_NAME_FUNC, KLC_NL_FUNC_DUMP);

    if (! name) {
        RETURN(BS_BAD_PTR);
    }

    func.type = KLC_FUNC_TYPE_NAME;
    func.id = -1;
    strlcpy(func.hdr.name, name, sizeof(func.hdr.name));

    return NetLink_Do(_klctool_func_dump, cmd, &func, sizeof(func));
}

int KLCTOOL_KoVersion()
{
    int cmd = KLC_NL_TYPE_CMD(KLC_NL_TYPE_CONFIG, KLC_NL_KO_VER);
    return NetLink_Do(_klctool_common_cmd, cmd, NULL, 0);
}

int KLCTOOL_IncUse()
{
    int cmd = KLC_NL_TYPE_CMD(KLC_NL_TYPE_CONFIG, KLC_NL_KO_INCUSE);
    return NetLink_Do(_klctool_common_cmd, cmd, NULL, 0);
}

int KLCTOOL_DecUse()
{
    int cmd = KLC_NL_TYPE_CMD(KLC_NL_TYPE_CONFIG, KLC_NL_KO_DECUSE);
    return NetLink_Do(_klctool_common_cmd, cmd, NULL, 0);
}

