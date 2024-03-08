/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "linux/kernel.h"
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

static int _ebpf_auto_set_prog_type(void *prog)
{
    const char *event = bpf_program__section_name(prog);
	enum bpf_prog_type prog_type = 0;

    if (! event) {
		printf("Has no sec name\n");
        return -1;
    }

	bool is_socket = strncmp(event, "socket", 6) == 0;
	bool is_kprobe = strncmp(event, "kprobe/", 7) == 0;
	bool is_kretprobe = strncmp(event, "kretprobe/", 10) == 0;
	bool is_tracepoint = strncmp(event, "tracepoint/", 11) == 0;
	bool is_raw_tracepoint = strncmp(event, "raw_tracepoint/", 15) == 0;
	bool is_xdp = strncmp(event, "xdp", 3) == 0;
	bool is_perf_event = strncmp(event, "perf_event", 10) == 0;
	bool is_cgroup_skb = strncmp(event, "cgroup/skb", 10) == 0;
	bool is_cgroup_sk = strncmp(event, "cgroup/sock", 11) == 0;
	bool is_sockops = strncmp(event, "sockops", 7) == 0;
	bool is_sk_skb = strncmp(event, "sk_skb", 6) == 0;
	bool is_sk_msg = strncmp(event, "sk_msg", 6) == 0;

	if (is_socket) {
		prog_type = BPF_PROG_TYPE_SOCKET_FILTER;
	} else if (is_kprobe || is_kretprobe) {
		prog_type = BPF_PROG_TYPE_KPROBE;
	} else if (is_tracepoint) {
		prog_type = BPF_PROG_TYPE_TRACEPOINT;
	} else if (is_raw_tracepoint) {
		prog_type = BPF_PROG_TYPE_RAW_TRACEPOINT;
	} else if (is_xdp) {
		prog_type = BPF_PROG_TYPE_XDP;
	} else if (is_perf_event) {
		prog_type = BPF_PROG_TYPE_PERF_EVENT;
	} else if (is_cgroup_skb) {
		prog_type = BPF_PROG_TYPE_CGROUP_SKB;
	} else if (is_cgroup_sk) {
		prog_type = BPF_PROG_TYPE_CGROUP_SOCK;
	} else if (is_sockops) {
		prog_type = BPF_PROG_TYPE_SOCK_OPS;
	} else if (is_sk_skb) {
		prog_type = BPF_PROG_TYPE_SK_SKB;
	} else if (is_sk_msg) {
		prog_type = BPF_PROG_TYPE_SK_MSG;
	} else {
		printf("Unknown event '%s'\n", event);
		return -1;
	}

	bpf_program__set_type(prog, prog_type);

    return 0;
}


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
    
    close(fd);
}

int EBPF_Load(void *obj)
{
    if (bpf_object__load(obj) < 0) {
        return ERR_Set(BS_CAN_NOT_OPEN, "Load object failed");
    }
    return 0;
}

void EBPF_AutoSetType(void *obj)
{
    void *prog = NULL;

	while ((prog = bpf_object__next_program(obj, prog))) {
        _ebpf_auto_set_prog_type(prog);
    }
}

int EBPF_AttachAuto(void *obj, OUT void **links, int links_max)
{
	struct bpf_program *prog;
    void *tmp[256];
    int cnt = 0;
    int err;
    const char *sec_name;

    if (! links) {
        links = tmp;
        links_max = ARRAY_SIZE(tmp);
    }

	bpf_object__for_each_program(prog, obj) {
        sec_name = bpf_program__section_name(prog);

        if (strncmp(sec_name, "klc/", sizeof("klc/")-1) == 0) {
            continue;
        }

		links[cnt] = bpf_program__attach(prog);
		err = libbpf_get_error(links[cnt]);
		if (err < 0) {
			fprintf(stderr, "ERROR: sec_name %s attach failed\n", sec_name);
			links[cnt] = NULL;
            EBPF_DetachLinks(links, cnt);
			return err;
		}
		cnt++;
        if (cnt >= links_max) {
            break;
        }
	}

    return cnt;
}

void * EBPF_LoadFile(char *path)
{
    void *obj;

    obj = EBPF_Open(path);
    if (! obj) {
        return NULL;
    }

    EBPF_AutoSetType(obj);

    if (EBPF_Load(obj) < 0) {
        EBPF_Close(obj);
        return NULL;
    }

    if (EBPF_AttachAuto(obj, NULL, 0) < 0) {
        EBPF_Close(obj);
        return NULL;
    }

    return obj;
}

void EBPF_DetachLinks(void **links, int links_cnt)
{
    int cnt = links_cnt;

	while (cnt) {
		bpf_link__destroy(links[cnt]);
        links[cnt] = NULL;
        cnt --;
    }
}

int EBPF_GetProgFdByName(void *obj, char *name)
{
    void *prog = bpf_object__find_program_by_name(obj, name);
    if (! prog) {
        return -1;
    }
    return bpf_program__fd(prog);
}

int EBPF_GetInfoByFd(int fd, OUT void *info, INOUT U32 *info_len)
{
    return bpf_obj_get_info_by_fd(fd, info, info_len);
}

int EBPF_PinMapByName(void *obj, char *map_name, char *path)
{
    void *map = bpf_object__find_map_by_name(obj, map_name);
    if (! map) {
        return -1;
    }

    return bpf_map__pin(map, path);
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

void * EBPF_GetMapByName(void *obj, const char *name)
{
    return bpf_object__find_map_by_name(obj, name);
}

int EBPF_GetMapFdByName(void *obj, const char *name)
{
    return bpf_object__find_map_fd_by_name(obj, name);
}


int EBPF_GetXdpAttachedFd(char *ifname)
{
    unsigned int fd;
    int ret;
    unsigned int ifindex;

    ifindex = if_nametoindex(ifname);
    if (! ifindex) {
        return ERR_VSet(BS_NO_SUCH, "Can't get %s index", ifname);
    }

    ret = bpf_xdp_query_id(ifindex, XDP_FLAGS_SKB_MODE, &fd);
    if (ret < 0) {
        return -1;
    }

    return fd;
}

int EBPF_AttachXdp(char *ifname, int fd, unsigned int flags)
{
    unsigned int ifindex;
    int ret;

    ifindex = if_nametoindex(ifname);
    if (! ifindex) {
        return ERR_VSet(BS_NO_SUCH, "Can't get %s index", ifname);
    }

    ret = bpf_xdp_attach(ifindex, fd, flags, NULL);
    if (ret < 0) {
        return ERR_Set(ret, "attach failed");
    }

    return 0;
}

int EBPF_DetachXdp(char *ifname, unsigned int flags)
{
    unsigned int ifindex;

    ifindex = if_nametoindex(ifname);
    if (! ifindex) {
        return ERR_VSet(BS_NO_SUCH, "Can't get %s index", ifname);
    }

    bpf_xdp_detach(ifindex, XDP_FLAGS_SKB_MODE, NULL);

    return 0;
}

#if 0
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
#endif
