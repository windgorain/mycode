/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/ebpf_utl.h"
#include "utl/cff_utl.h"
#include "utl/file_utl.h"
#include "utl/elf_utl.h"
#include "utl/subcmd_utl.h"
#include "utl/getopt2_utl.h"
#include "utl/netlink_utl.h"
#include "utl/bpf_map.h"
#include "bpf/libbpf.h"
#include "bpf/bpf.h"
#include "bpf/bpf_load.h"

void * EBPF_Open(char *filename)
{
    void *obj = bpf_object__open(filename);
    long err = libbpf_get_error(obj);
    if (err) {
        return NULL;
    }
    return obj;
}

void EBPF_Close(void *obj)
{
    bpf_object__close(obj);
}

int EBPF_GetFd(char *pin_filename)
{
    return bpf_obj_get(pin_filename);
}

void EBPF_CloseFd(int fd)
{
    /* 关闭通过EBPF_GetFd()获取到的fd */
    close(fd);
}

int EBPF_Load(void *obj)
{
    if (bpf_object__load(obj) < 0) {
        return ERR_Set(BS_CAN_NOT_OPEN, "Load object failed");
    }
    return 0;
}

int EBPF_GetProgFdByName(void *obj, char *name)
{
    void *prog = bpf_object__find_program_by_name(obj, name);
    if (! prog) {
        return -1;
    }
    return bpf_program__fd(prog);
}

int EBPF_PinProgs(void *obj, char *path)
{
    if (0 != FILE_MakePath(path)) {
        return ERR_Set(BS_ERR, "Create prog pin path failed");
    }

    if (bpf_object__pin_programs(obj, path) < 0) {
        return ERR_Set(BS_ERR, "Pin progs failed");
    }

    return 0;
}

int EBPF_PinMaps(void *obj, char *path)
{
    if (0 != FILE_MakePath(path)) {
        return ERR_Set(BS_ERR, "Create map pin path failed");
    }

    if (bpf_object__pin_maps(obj, path) < 0) {
        return ERR_Set(BS_ERR, "Pin maps failed");
    }

    return 0;
}

int EBPF_Pin(void *obj, char *prog_path, char *map_path)
{
    int ret;

    ret = EBPF_PinProgs(obj, prog_path);
    if (ret < 0) {
        return ret;
    }

    ret = EBPF_PinMaps(obj, map_path);
    if (ret < 0) {
        EBPF_UnPinProgs(obj, prog_path);
        return ret;
    }

    return 0;
}

void EBPF_UnPinProgs(void *obj, char *path)
{
    bpf_object__unpin_programs(obj, path);
}

void EBPF_UnPinMaps(void *obj, char *path)
{
    bpf_object__unpin_maps(obj, path);
}

int EBPF_GetProgFdById(unsigned int bpf_id)
{
    return bpf_prog_get_fd_by_id(bpf_id);
}

int EBPF_GetMapFdById(unsigned int bpf_id)
{
    return bpf_map_get_fd_by_id(bpf_id);
}

/* 获取attach到ifname上的xdp fd  */
int EBPF_GetXdpAttachedFd(char *ifname)
{
    unsigned int fd;
    int ret;
    unsigned int ifindex;

    ifindex = if_nametoindex(ifname);
    if (! ifindex) {
        return ERR_VSet(BS_NO_SUCH, "Can't get %s index", ifname);
    }

    ret = bpf_get_link_xdp_id(ifindex, &fd, XDP_FLAGS_SKB_MODE);
    if (ret < 0) {
        return -1;
    }

    return fd;
}

int EBPF_AttachXdp(char *ifname, int fd, UINT flags)
{
    unsigned int ifindex;
    int ret;

    ifindex = if_nametoindex(ifname);
    if (! ifindex) {
        return ERR_VSet(BS_NO_SUCH, "Can't get %s index", ifname);
    }

    ret = bpf_set_link_xdp_fd(ifindex, fd, flags);
    if (ret < 0) {
        return ERR_Set(ret, "attach failed");
    }

    return 0;
}

int EBPF_DetachXdp(char *ifname)
{
    unsigned int ifindex;

    ifindex = if_nametoindex(ifname);
    if (! ifindex) {
        return ERR_VSet(BS_NO_SUCH, "Can't get %s index", ifname);
    }

    bpf_set_link_xdp_fd(ifindex, -1, XDP_FLAGS_SKB_MODE);

    return 0;
}

int EBPF_GetMapAttr(int fd, OUT EBPF_MAP_ATTR_S *attr)
{
    union bpf_attr bpfattr;
    int ret;

    memset(&bpfattr, 0, sizeof(bpfattr));

    ret = bpf_obj_get_attr_by_fd(fd, &bpfattr, sizeof(bpfattr));
    if (ret < 0) {
        BS_PRINT_ERR("ret=%d\r\n", ret);
        return ret;
    }

    attr->map_type = bpfattr.map_type;
    attr->key_size = bpfattr.key_size;
    attr->value_size = bpfattr.value_size;
    attr->max_elem = bpfattr.max_entries;
    attr->flags = bpfattr.map_flags;

    return 0;
}

