/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/jhash_utl.h"
#include "utl/ebpf_utl.h"
#include "utl/consistent_hash.h"
#include "utl/file_utl.h"
#include "utl/cff_utl.h"
#include "utl/elf_utl.h"
#include "utl/subcmd_utl.h"
#include "utl/getopt2_utl.h"
#include "utl/netlink_utl.h"
#include "utl/str_num.h"
#include "utl/bpf_map.h"
#include "bpf/libbpf.h"
#include "bpf/bpf.h"
#include "bpf/bpf_load.h"
#include "utl/bpf_map.h"
#include "klc/mapptool_lib.h"
#include "klc/klctool_lib.h"
#include "app/mapp/mapp_def.h"

typedef struct {
    int mapp_prog_fd;
    int slot_map_fd; /* 存放slot信息的map */
    int name_slot_map_fd; /* 存放name -> slot映射的map */
    int prog_map_fd; /* prog map */
    int vslot_map_fd; /* 用于一致性hash的表 */
}MAPPTOOL_ATTACH_FD_S;

typedef struct {
    char file[256];
    int type;
    int class;
    char prog[128];
}MAPPTOOL_MAPP_CFG_S;

/* 获取mapp的pin目录 */
static inline int _mapptool_build_mapp_pin_path(char *instance, OUT char *mapp_pin_path, int size)
{
    return SNPRINTF(mapp_pin_path, size, "%s/%s", MAPPTOOL_PIN_PATH, instance);
}

/* 获取mapp progs的pin目录 */
static inline int _mapptool_build_prog_pin_path(char *instance, OUT char *prog_pin_path, int size)
{
    return SNPRINTF(prog_pin_path, size, "%s/%s/progs", MAPPTOOL_PIN_PATH, instance);
}

/* 获取mapp maps的pin目录 */
static inline int _mapptool_build_map_pin_path(char *instance, OUT char *map_pin_path, int size)
{
    return SNPRINTF(map_pin_path, size, "%s/%s/maps", MAPPTOOL_PIN_PATH, instance);
}

/* 获取mapp prog的pin文件路径 */
static inline int _mapptool_build_prog_pin_file(char *instance, char *prog, OUT char *prog_pin_file, int size)
{
    return SNPRINTF(prog_pin_file, size, "%s/%s/progs/%s", MAPPTOOL_PIN_PATH, instance, prog);
}

/* 获取mapp map的pin文件路径 */
static inline int _mapptool_build_map_pin_file(char *instance, char *map_name, OUT char *map_pin_file, int size)
{
    return SNPRINTF(map_pin_file, size, "%s/%s/maps/%s", MAPPTOOL_PIN_PATH, instance, map_name);
}

static inline void _mapptool_close_fd(int fd)
{
    if (fd >= 0) {
        EBPF_CloseFd(fd);
    }
}

/* 关闭fds */
static void _mapptool_close_fds(MAPPTOOL_ATTACH_FD_S *fds)
{
    _mapptool_close_fd(fds->mapp_prog_fd);
    _mapptool_close_fd(fds->slot_map_fd);
    _mapptool_close_fd(fds->prog_map_fd);
    _mapptool_close_fd(fds->name_slot_map_fd);
    _mapptool_close_fd(fds->vslot_map_fd);
}

/* 获取mapp prog的fd */
static inline int _mapptool_open_mapp_prog_fd(char *instance, char *prog_name)
{
    char prog_file[MAPPTOOL_PIN_PATH_SIZE];
    int ret;
    int fd;

    ret = _mapptool_build_prog_pin_file(instance, prog_name, prog_file, sizeof(prog_file));
    if (ret < 0) {
        return ERR_Set(BS_OUT_OF_RANGE, "Can't build pin path");
    }

    fd = EBPF_GetFd(prog_file);
    if (fd < 0) {
        return ERR_VSet(fd, "Can't get prog %s:%s", instance, prog_name);
    }

    return fd;
}

/* open fds */
static int _mapptool_open_fds(char *instance, char *prog_name, char *attach_name, OUT MAPPTOOL_ATTACH_FD_S *fds)
{
    if (instance) {
        fds->mapp_prog_fd = _mapptool_open_mapp_prog_fd(instance, prog_name);
        if (fds->mapp_prog_fd < 0) {
            return BS_ERR;
        }
    }

    fds->slot_map_fd = MAPPTOOL_OpenMap(attach_name, MAPP_SLOT_MAP_NAME);
    fds->name_slot_map_fd = MAPPTOOL_OpenMap(attach_name, MAPP_NAME_SLOT_MAP_NAME);
    fds->prog_map_fd = MAPPTOOL_OpenMap(attach_name, MAPP_PROG_MAP_NAME);

    if ((fds->slot_map_fd < 0) || (fds->prog_map_fd < 0) || (fds->name_slot_map_fd < 0)) {
        _mapptool_close_fds(fds);
        return BS_ERR;
    }

    fds->vslot_map_fd = MAPPTOOL_OpenMap(attach_name, MAPP_VSLOT_MAP_NAME);
    ErrCode_Clear();

    return 0;
}

/* 根据mapp name查找slot */
static int _mapptool_get_slot_by_instance(MAPPTOOL_ATTACH_FD_S *fds, char *instance)
{
    char name[MAPP_NAME_SIZE];
    int slot;

    memset(name, 0, sizeof(name));
    strlcpy(name, instance, sizeof(name));

    if (BPF_LookupMapEle(fds->name_slot_map_fd, name, &slot) < 0) {
        return -1;
    }

    return slot;
}

/* 获取slot data */
static int _mapptool_get_slot_data(MAPPTOOL_ATTACH_FD_S *fds, int slot, OUT MAPP_SLOT_DATA_S *slot_data)
{
    if (slot < 0) {
        return -1;
    }

    if (BPF_LookupMapEle(fds->slot_map_fd, &slot, slot_data) < 0) {
        return -1;
    }

    return 0;
}

/* 判断slot是否已经被使用 */
static BOOL_T _mapptool_is_slot_used(MAPPTOOL_ATTACH_FD_S *fds, int slot)
{
    MAPP_SLOT_DATA_S slot_data;

    if (_mapptool_get_slot_data(fds, slot, &slot_data) < 0) {
        return FALSE;
    }

    if (slot_data.attached_name[0] == '\0') {
        return FALSE;
    }

    return TRUE;
}

/* 分配一个slot */
static int _mapptool_alloc_slot(MAPPTOOL_ATTACH_FD_S *fds, char *instance, int slot /* -1表示自动分配 */)
{
    int i;
    MAPP_SLOT_DATA_S slot_data = {0};

    if (slot < 0) {
        for (i=0; i<MAPP_SLOT_MAX; i++) {
            if (! _mapptool_is_slot_used(fds, i)) {
                slot = i;
                break;
            }
        }
    }

    if (slot < 0) {
        return -1;
    }

    strlcpy(slot_data.attached_name, instance, MAPP_NAME_SIZE);

    if (BPF_UpdateMapEle(fds->slot_map_fd, &slot, &slot_data, BPF_ANY) < 0) {
        return -1;
    }

    if (BPF_UpdateMapEle(fds->name_slot_map_fd, slot_data.attached_name, &slot, BPF_ANY) < 0) {
        BPF_DelMapEle(fds->slot_map_fd, &slot);
        return -1;
    }

    return slot;
}

/* 将slot删除 */
static void _mapptool_remove_slot(MAPPTOOL_ATTACH_FD_S *fds, int slot, char *instance)
{
    char name[MAPP_NAME_SIZE];

    memset(name, 0, sizeof(name));
    strlcpy(name, instance, sizeof(name));

    if (slot >= MAPP_SLOT_MAX) {
        return;
    }

    BPF_DelMapEle(fds->slot_map_fd, &slot);
    BPF_DelMapEle(fds->name_slot_map_fd, name);
}


static int _mapptool_build_vslots(INOUT MAPP_VSLOT_S *vslots, int slot)
{
    int i;
    UINT hash;

    vslots->chash.max = MAPP_VSLOT_MAX;

    for (i=0; i<MAPP_VSLOT_PER_SLOT; i++) {
        hash = JHASH_GeneralBuffer(&slot, sizeof(int), i);
        ConsistentHash_AddNode(&vslots->chash, hash, slot);
    }

    return 0;
}

static int _mapptool_process_vslot(MAPPTOOL_ATTACH_FD_S *fds, int slot)
{
    MAPP_VSLOT_S vslots = {0};
    int key = 0;

    if (fds->vslot_map_fd < 0) {
        return -1;
    }

    BPF_LookupMapEle(fds->vslot_map_fd, &key, &vslots);

    _mapptool_build_vslots(&vslots, slot);

    return BPF_UpdateMapEle(fds->vslot_map_fd, &key, &vslots, BPF_ANY);
}

static int _mapptool_attach_instance(char *instance, int slot, MAPPTOOL_ATTACH_FD_S *fds)
{
    int ret;

    /* 检查指定的slot是否已经被占用 */
    if ((slot >= 0) && (_mapptool_is_slot_used(fds, slot) > 0)){
        return ERR_Set(BS_BUSY, "Slot has been used");
    }

    /* 判断是否已经存在 */
    if (_mapptool_get_slot_by_instance(fds, instance) >= 0) {
        return ERR_Set(BS_ALREADY_EXIST, "the instance already exist");
    }

    slot = _mapptool_alloc_slot(fds, instance, slot);
    if (slot< 0) {
        return ERR_Set(BS_ERR, "Can't alloc slot");
    }

    ret = BPF_UpdateMapEle(fds->prog_map_fd, &slot, &fds->mapp_prog_fd, BPF_ANY);
    if (ret < 0) {
        _mapptool_remove_slot(fds, slot, instance);
        return ERR_Set(ret, "Can't attach to prog map");
    }

    _mapptool_process_vslot(fds, slot);

    return 0;
}

static int _mapptool_detach_instance(char *instance, MAPPTOOL_ATTACH_FD_S *fds)
{
    int slot;

    slot = _mapptool_get_slot_by_instance(fds, instance);
    if (slot < 0) {
        return ERR_Set(BS_NOT_FOUND, "Can't find slot for this instance");
    }

    _mapptool_remove_slot(fds, slot, instance);

    BPF_DelMapEle(fds->prog_map_fd, &slot);

    return 0;
}

static int _mapptool_detach_instance_all(MAPPTOOL_ATTACH_FD_S *fds)
{
    int slot;
    int ret;
    MAPP_SLOT_DATA_S slot_data;

    for (slot=0; slot<MAPP_SLOT_MAX; slot++) {
        ret = BPF_LookupMapEle(fds->slot_map_fd, &slot, &slot_data);
        if (ret < 0) {
            continue;
        }
        _mapptool_remove_slot(fds, slot, slot_data.attached_name);
        BPF_DelMapEle(fds->prog_map_fd, &slot);
    }

    return 0;
}

/* 从文件中获取attr信息 */
static int _mapptool_get_obj_attr_sec(char *file, OUT MAPP_ATTR_S *attr)
{
    ELF_S elf = {0};
    ELF_SECTION_S sec;
    int ret;

    ret = ELF_Open(file, &elf);
    if (ret < 0) {
        return -1;
    }

    if (ELF_GetSecByName(&elf, MAPP_ATTR_SEC_NAME, &sec) < 0) {
        ELF_Close(&elf);
        return -1;
    }

    memcpy(attr, sec.data->d_buf, MIN(sizeof(*attr), sec.data->d_size));

    ELF_Close(&elf);

    return 0;
}

/* 从文件中获取attr的一些信息并写到instance的map中 */
static int _mapptool_load_mapp_attr(char *file, char *instance, char *description)
{
    int ret;
    ELF_S elf = {0};
    ELF_SECTION_S sec;
    MAPP_ATTR_S attr = {0};

    ret = ELF_Open(file, &elf);
    if (ret < 0) {
        return -1;
    }

    if (ELF_GetSecByName(&elf, MAPP_ATTR_SEC_NAME, &sec) < 0) {
        ELF_Close(&elf);
        return -1;
    }

    memcpy(&attr, sec.data->d_buf, MIN(sizeof(attr), sec.data->d_size));
    strlcpy(attr.instance_name, instance, sizeof(attr.instance_name));
    if (description) {
        strlcpy(attr.description, description, sizeof(attr.description));
    }

    ret = MAPPTOOL_SetAttr(instance, &attr);

    ELF_Close(&elf);

    return ret;
}

static int _mapptool_unload_normal_instance(char *instance)
{
    char mapp_pin_path[MAPPTOOL_PIN_PATH_SIZE];
    int ret;

    ret = _mapptool_build_mapp_pin_path(instance, mapp_pin_path, sizeof(mapp_pin_path));
    if (ret < 0) {
        return ERR_Set(BS_OUT_OF_RANGE, "Can't build mapp path");
    }

    return FILE_DelDir(mapp_pin_path);
}

static int _mapptool_unload_root_instance(char *instance, MAPP_ATTR_S *attr)
{
    KLCTOOL_DelModule(attr->mapp_name);
    return 0;
}

static int _mapptool_unload_instance(char *instance, MAPP_ATTR_S *attr)
{
    if (attr->class == MAPP_CLASS_ROOT) {
        _mapptool_unload_root_instance(instance, attr);
    }

    return _mapptool_unload_normal_instance(instance);
}

char * MAPPTOOL_Type2String(int mapp_type)
{
    char *str;
    static STR_ID_S nodes[] = {
        {.str="unknown", .id=MAPP_TYPE_UNKNOWN},
        {.str="none", .id=MAPP_TYPE_NONE},
        {.str="noctx", .id=MAPP_TYPE_NOCTX},
        {.str="xdp", .id=MAPP_TYPE_XDP},
        {.str="tc", .id=MAPP_TYPE_TC},
        {.str=NULL}
    };

    str = StrID_ID2Str(nodes, mapp_type);
    if (! str) {
        str = "";
    }

    return str;
}

char * MAPPTOOL_Class2String(int mapp_class)
{
    char *str;
    static STR_ID_S nodes[] = {
        {.str="unknown", .id=MAPP_CLASS_UNKNOWN},
        {.str="leaf", .id=MAPP_CLASS_LEAF},
        {.str="traffic", .id=MAPP_CLASS_TRAFFIC},
        {.str="cluster", .id=MAPP_CLASS_CLUSTER},
        {.str="pipe", .id=MAPP_CLASS_PIPE},
        {.str="mirror", .id=MAPP_CLASS_MIRROR},
        {.str="root", .id=MAPP_CLASS_ROOT},
        {.str="base", .id=MAPP_CLASS_BASE},
        {.str=NULL}
    };

    str = StrID_ID2Str(nodes, mapp_class);
    if (! str) {
        str = "";
    }

    return str;
}

static int _mapptool_load_obj(void *obj, char *file, char *instance, char *description)
{
    int ret;
    char prog_pin_path[MAPPTOOL_PIN_PATH_SIZE];
    char map_pin_path[MAPPTOOL_PIN_PATH_SIZE];

    ret = _mapptool_build_prog_pin_path(instance, prog_pin_path, sizeof(prog_pin_path));
    ret |= _mapptool_build_map_pin_path(instance, map_pin_path, sizeof(map_pin_path));
    if (ret < 0) {
        return ERR_Set(BS_OUT_OF_RANGE, "Can't build pin path");
    }

    ret = EBPF_Load(obj);
    if (ret < 0) {
        return ERR_Set(BS_CAN_NOT_OPEN, "Can't load object file");
    }

    ret = EBPF_Pin(obj, prog_pin_path, map_pin_path);
    if (ret < 0) {
        return ERR_Set(BS_CAN_NOT_OPEN, "Can't pin obj");
    }

    return _mapptool_load_mapp_attr(file, instance, description);
}

static int _mapptool_load_normal(char *file, char *instance, char *description)
{
    void *obj;
    int ret;

    obj = EBPF_Open(file);
    if (! obj) {
        return ERR_Set(BS_CAN_NOT_OPEN, "Can't open object file");
    }

    ret = _mapptool_load_obj(obj, file, instance, description);

    EBPF_Close(obj);

    return ret;
}

static int mapptool_load_root(char *file, char *instance, char *description)
{
    int ret;
    char mapp_pin_path[MAPPTOOL_PIN_PATH_SIZE];

    ret = _mapptool_build_map_pin_path(instance, mapp_pin_path, sizeof(mapp_pin_path));
    if (ret < 0) {
        return ERR_Set(BS_OUT_OF_RANGE, "Can't build mapp map path");
    }

    ret = KLCTOOL_LoadFile(file, 0, NULL, mapp_pin_path);
    if (ret < 0) {
        return ERR_Set(BS_ERR, "Can't load mapp instance");
    }

    return _mapptool_load_mapp_attr(file, instance, description);
}

int MAPPTOOL_Load(char *file, char *instance, char *description)
{
    int ret;

    MAPP_ATTR_S attr = {0};

    ret = _mapptool_get_obj_attr_sec(file, &attr);
    if (ret < 0) {
        return _mapptool_load_normal(file, instance, description);
    }

    if (attr.class == MAPP_CLASS_ROOT) {
        return mapptool_load_root(file, instance, description);
    }

    return _mapptool_load_normal(file, instance, description);
}

int MAPPTOOL_UnLoad(char *instance)
{
    MAPP_ATTR_S attr = {0};

    if (MAPPTOOL_GetAttr(instance, &attr) < 0) {
        return ERR_Set(BS_CAN_NOT_OPEN, "Can't get instance attr");
    }

    return _mapptool_unload_instance(instance, &attr);
}

/* 获取mapp map的fd */
int MAPPTOOL_OpenMap(char *instance, char *map_name)
{
    char map_file[MAPPTOOL_PIN_PATH_SIZE];
    int ret;
    int fd;

    ret = _mapptool_build_map_pin_file(instance, map_name, map_file, sizeof(map_file));
    if (ret < 0) {
        return ERR_Set(BS_OUT_OF_RANGE, "Can't build pin path");
    }

    fd = EBPF_GetFd(map_file);
    if (fd < 0) {
        return ERR_VSet(fd, "Can't get map %s:%s", instance, map_name);
    }

    return fd;
}

int MAPPTOOL_SetAttr(char *instance, MAPP_ATTR_S *attr)
{
    int key = 0;
    int ret;

    int fd = MAPPTOOL_OpenMap(instance, MAPP_ATTR_MAP_NAME);
    if (fd < 0) {
        return fd;
    }

    ret = BPF_UpdateMapEle(fd, &key, attr, BPF_ANY);

    EBPF_CloseFd(fd);

    return ret;
}

int MAPPTOOL_GetAttr(char *instance, OUT MAPP_ATTR_S *attr)
{
    int key = 0;
    int ret;

    int fd = MAPPTOOL_OpenMap(instance, MAPP_ATTR_MAP_NAME);
    if (fd < 0) {
        return fd;
    }

    ret = BPF_LookupMapEle(fd, &key, attr);

    EBPF_CloseFd(fd);

    return ret;
}

int MAPPTOOL_AttachXdp(char *instance, char *prog_name, char *ifname)
{
    int ret;
    int prog_fd;

    prog_fd = _mapptool_open_mapp_prog_fd(instance, prog_name);
    if (prog_fd < 0) {
        return prog_fd;
    }

    ret = EBPF_AttachXdp(ifname, prog_fd, XDP_FLAGS_SKB_MODE);

    EBPF_CloseFd(prog_fd);

    return ret;
}

int MAPPTOOL_DetachXdp(char *ifname)
{
    return EBPF_DetachXdp(ifname);
}

int MAPPTOOL_AttachInstance(char *instance, char *prog_name, char *attach_to, int slot)
{
    int ret;
    MAPP_ATTR_S attr1;
    MAPP_ATTR_S attr2;
    MAPPTOOL_ATTACH_FD_S fds = {0};

    ret = MAPPTOOL_GetAttr(attach_to, &attr1);
    if (ret < 0) {
        return ERR_VSet(BS_ERR, "Can't get instance %s attr", attach_to);
    }

    ret = MAPPTOOL_GetAttr(instance, &attr2);
    if (ret < 0) {
        return ERR_VSet(BS_ERR, "Can't get instance %s attr", instance);
    }

    if (attr1.out_type != attr2.in_type) {
        return ERR_VSet(BS_ERR, "in/out type not match, out_type:%s, in_type:%s",  
                MAPPTOOL_Type2String(attr1.out_type),
                MAPPTOOL_Type2String(attr2.in_type));
    }

    ret = _mapptool_open_fds(instance, prog_name, attach_to, &fds);
    if (ret < 0) {
        return ret;
    }

    ret = _mapptool_attach_instance(instance, slot, &fds);

    _mapptool_close_fds(&fds);

    return ret;
}

int MAPPTOOL_DetachInstance(char *instance, char *detach_from)
{
    int ret;
    MAPPTOOL_ATTACH_FD_S fds = {0};

    ret = _mapptool_open_fds(NULL, NULL, detach_from, &fds);
    if (ret < 0) {
        return ret;
    }

    ret = _mapptool_detach_instance(instance, &fds);

    _mapptool_close_fds(&fds);

    return ret;
}

/* detach all from instance x */
int MAPPTOOL_DetachInstanceAll(char *detach_from)
{
    int ret;
    MAPPTOOL_ATTACH_FD_S fds = {0};

    ret = _mapptool_open_fds(NULL, NULL, detach_from, &fds);
    if (ret < 0) {
        return ret;
    }

    ret = _mapptool_detach_instance_all(&fds);

    _mapptool_close_fds(&fds);

    return ret;
}

int MAPPTOOL_GetSlotNum(char *instance)
{
    return MAPP_SLOT_MAX;
}

int MAPPTOOL_GetSlotData(char *instance, int slot, OUT MAPP_SLOT_DATA_S *data)
{
    int ret;
    MAPPTOOL_ATTACH_FD_S fds = {0};

    ret = _mapptool_open_fds(NULL, NULL, instance, &fds);
    if (ret < 0) {
        return ret;
    }

    ret = BPF_LookupMapEle(fds.slot_map_fd, &slot, data);

    _mapptool_close_fds(&fds);

    return ret;
}

