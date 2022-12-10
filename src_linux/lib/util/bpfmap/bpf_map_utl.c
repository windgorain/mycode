#include<stdio.h>
#include <libgen.h>
#include <sys/vfs.h>
#include "utl/bpf_map.h"
#include "bpf/bpf.h"
#include "utl/txt_utl.h"
#include "utl/file_utl.h"

static int bpf_make_parent_dir(char *filename)
{
    char path[512] = "";

    FILE_GetPathFromFilePath(filename, path);

    return FILE_MakePath(path);
}

static int bpf_check_path(const char *path)
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

int BPF_CreateMap(IN BPF_MAP_PARAM_S *p)
{
    int fd = -1;
    int ret;

    if (p->filename[0]) {
        fd = bpf_obj_get(p->filename);
    }

    if (fd < 0) {
        fd = bpf_create_map(p->type, p->key_size, p->value_size, p->max_elem, p->flags);
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

	err = bpf_make_parent_dir(filename);
	if (err) {
        return err;
    }

    err = bpf_check_path(filename);
    if (err) {
        return err;
    }

    return bpf_obj_pin(fd, filename);
}

int BPF_LookupMapEle(int fd, void *key, void *val)
{
    if (fd < 0) return -1;
    if (! key) return -1;

    return bpf_map_lookup_elem(fd, key, val);
}

int BPF_DelMapEle(int fd, void *key)
{
    if (fd < 0) return -1;
    if (! key) return -1;

    return bpf_map_delete_elem(fd, key);
}

int BPF_UpdateMapEle(int fd, void *key, void *val, int flags)
{
    if (fd < 0) return -1;
    if (! key) return -1;
    if (! val) return -1;

    return bpf_map_update_elem(fd, key, val, flags);
}

int BPF_GetNextMapKey(int fd, const void *key, void *next_key)
{
    return bpf_map_get_next_key(fd, key, next_key);
}
