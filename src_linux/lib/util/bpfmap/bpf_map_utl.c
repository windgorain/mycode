#include<stdio.h>
#include <asm/unistd.h>
#include <libgen.h>
#include <sys/vfs.h>
#include "utl/bpf_map.h"
#include "bpf/bpf.h"
#include "utl/txt_utl.h"
#include "utl/file_utl.h"

static inline int _sys_bpf(int cmd, union bpf_attr *attr, unsigned int size)
{
	return syscall(__NR_bpf, cmd, attr, size);
}

static U64 _ptr_to_u64(const void *ptr)
{
	return (U64) (unsigned long) ptr;
}

static inline int _libbpf_err_errno(int ret)
{
	/* errno is already assumed to be set on error */
	return ret < 0 ? -errno : ret;
}

static int _bpf_obj_get(const char *pathname)
{
	const size_t attr_sz = offsetofend(union bpf_attr, file_flags);
	union bpf_attr attr;

	memset(&attr, 0, attr_sz);
	attr.pathname = _ptr_to_u64((void *)pathname);

	return _sys_bpf(BPF_OBJ_GET, &attr, attr_sz);
}

static int _bpf_obj_pin(int fd, const char *pathname)
{
	const size_t attr_sz = offsetofend(union bpf_attr, file_flags);
	union bpf_attr attr;

	memset(&attr, 0, attr_sz);
	attr.pathname = _ptr_to_u64((void *)pathname);
	attr.bpf_fd = fd;

	return _sys_bpf(BPF_OBJ_PIN, &attr, attr_sz);
}

static int _bpf_create_map(int map_type, int key_size, int value_size, int max_entries, int map_flags)
{
	union bpf_attr attr = {
		.map_type = map_type,
		.key_size = key_size,
		.value_size = value_size,
		.max_entries = max_entries,
		.map_flags = map_flags,
	};

	return _sys_bpf(BPF_MAP_CREATE, &attr, sizeof(attr));
}

static int _bpf_make_parent_dir(char *filename)
{
    char path[512] = "";

    FILE_GetPathFromFilePath(filename, path);

    return FILE_MakePath(path);
}

static int _bpf_check_path(const char *path)
{
    struct statfs st_fs;
    char *dname, *dir;
    int err = 0;

    if (path == NULL)
        return -EINVAL;

    dname = strdup(path);
    if (dname == NULL)
        return -ENOMEM;

    dir = dirname(dname);
    if (statfs(dir, &st_fs)) {
        err = -errno;
    }

    free(dname);

    return err;
}

static int _bpf_map_lookup_elem(int fd, const void *key, void *value)
{
	const size_t attr_sz = offsetofend(union bpf_attr, flags);
	union bpf_attr attr;
	int ret;

	memset(&attr, 0, attr_sz);
	attr.map_fd = fd;
	attr.key = _ptr_to_u64(key);
	attr.value = _ptr_to_u64(value);

	ret = _sys_bpf(BPF_MAP_LOOKUP_ELEM, &attr, attr_sz);
	return _libbpf_err_errno(ret);
}

static int _bpf_map_delete_elem(int fd, const void *key)
{
	const size_t attr_sz = offsetofend(union bpf_attr, flags);
	union bpf_attr attr;
	int ret;

	memset(&attr, 0, attr_sz);
	attr.map_fd = fd;
	attr.key = _ptr_to_u64(key);

	ret = _sys_bpf(BPF_MAP_DELETE_ELEM, &attr, attr_sz);
	return _libbpf_err_errno(ret);
}

static int _bpf_map_update_elem(int fd, const void *key, const void *value, __u64 flags)
{
	const size_t attr_sz = offsetofend(union bpf_attr, flags);
	union bpf_attr attr;
	int ret;

	memset(&attr, 0, attr_sz);
	attr.map_fd = fd;
	attr.key = _ptr_to_u64(key);
	attr.value = _ptr_to_u64(value);
	attr.flags = flags;

	ret = _sys_bpf(BPF_MAP_UPDATE_ELEM, &attr, attr_sz);
	return _libbpf_err_errno(ret);
}

static int _bpf_map_get_next_key(int fd, const void *key, void *next_key)
{
	const size_t attr_sz = offsetofend(union bpf_attr, next_key);
	union bpf_attr attr;
	int ret;

	memset(&attr, 0, attr_sz);
	attr.map_fd = fd;
	attr.key = _ptr_to_u64(key);
	attr.next_key = _ptr_to_u64(next_key);

	ret = _sys_bpf(BPF_MAP_GET_NEXT_KEY, &attr, attr_sz);
	return _libbpf_err_errno(ret);
}


int BPF_CreateMap(IN BPF_MAP_PARAM_S *p)
{
    int fd = -1;
    int ret;

    if (p->filename[0]) {
        fd = _bpf_obj_get(p->filename);
    }

    if (fd < 0) {
        fd = _bpf_create_map(p->type, p->key_size, p->value_size, p->max_elem, p->flags);
    }

    if (fd < 0) {
        return fd;
    }

    if (p->pinning && p->filename[0]) {
        ret = BPF_PinMap(fd, p->filename);
        if (ret < 0) {
            close(fd);
            return ret;
        }
    }

    return fd;
}

int BPF_NewMap(char *filename, int type, int key_size, int val_size, int max_elements, int flags)
{
    BPF_MAP_PARAM_S p = {0};

    if (filename) {
        strlcpy(p.filename, filename, sizeof(p.filename));
    }
    p.type = type;
    p.key_size = key_size;
    p.value_size = val_size;
    p.max_elem = max_elements;
    p.flags = flags;

    return BPF_CreateMap(&p);
}

void BPF_DelMap(int fd)
{
    if (fd >= 0) {
        close(fd);
    }
}

int BPF_PinMap(int fd, char *filename)
{
    int err;

    if (fd < 0) {
        return BS_BAD_PARA;
    }

	err = _bpf_make_parent_dir(filename);
	if (err) {
        return err;
    }

    err = _bpf_check_path(filename);
    if (err) {
        return err;
    }

    return _bpf_obj_pin(fd, filename);
}

int BPF_LookupMapEle(int fd, void *key, void *val)
{
    if (fd < 0) return -1;
    if (! key) return -1;

    return _bpf_map_lookup_elem(fd, key, val);
}

int BPF_DelMapEle(int fd, void *key)
{
    if (fd < 0) return -1;
    if (! key) return -1;

    return _bpf_map_delete_elem(fd, key);
}

int BPF_UpdateMapEle(int fd, void *key, void *val, int flags)
{
    if (fd < 0) return -1;
    if (! key) return -1;
    if (! val) return -1;

    return _bpf_map_update_elem(fd, key, val, flags);
}

/* get first时, key的内容是一个不存在的key即可 */
int BPF_GetNextMapKey(int fd, const void *key, void *next_key)
{
    return _bpf_map_get_next_key(fd, key, next_key);
}
