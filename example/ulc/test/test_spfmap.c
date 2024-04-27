/*********************************************************
*   Copyright (C) LiXingang
*
********************************************************/
#include "utl/ulc_user.h"

struct path {
	void *mnt;
	void *dentry;
};

SEC(".spf.cmd/")
int test_cmd()
{
    struct path path;
    int ret;

    ret = ulc_call_sym(-1, "kern_path", "/sys/fs/bpf/klc/mgw/acl1/maps/g_mgw_ipacl_tbl", 1, &path);
    if (ret != 0) {
        printf("Can't get map\n");
        return -1;
    }

    void *m = (void*)ulc_call_sym(0, "klc_get_path_private", &path);

    printf("map ptr = %p \n", m);

    ulc_call_sym(0, "path_put", &path);

    return 0;
}


