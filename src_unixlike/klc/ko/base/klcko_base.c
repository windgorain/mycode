/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "klcko_lib.h"

typedef struct bpf_func_proto* (*PF_klc_lsm_func_proto)(int func_id, const struct bpf_prog *prog);

static u64 _bpf_get_cgroup_classid(u64 type, u64 p2, u64 p3, u64 p4, u64 p5);

static PF_klc_lsm_func_proto g_klc_old_get_func_proto = NULL;
static struct bpf_verifier_ops g_klc_hook_ops;
static PF_KLC_HELPER g_klc_extern_func = NULL;

static const struct bpf_func_proto g_klcko_hook_proto = {
	.func		= _bpf_get_cgroup_classid,
	.gpl_only	= false,
	.ret_type	= RET_INTEGER,
	.arg1_type	= ARG_ANYTHING,
	.arg2_type	= ARG_ANYTHING,
	.arg3_type	= ARG_ANYTHING,
	.arg4_type	= ARG_ANYTHING,
	.arg5_type	= ARG_ANYTHING
};

static const void * _klcko_hook_get_func_proto(int id, void *param)
{
    if (id == 17) {
        return &g_klcko_hook_proto;
    }

    if (g_klc_old_get_func_proto) {
        return g_klc_old_get_func_proto(id, param);
    }

    return NULL;
}

static u64 _bpf_get_cgroup_classid(u64 type, u64 p2, u64 p3, u64 p4, u64 p5)
{
    PF_KLC_HELPER func;
    u64 ret = KLC_RET_ERR;

    rcu_read_lock();

    func = rcu_dereference(g_klc_extern_func);
    if (func) {
        ret = func(type, p2, p3, p4, p5);
    }

	rcu_read_unlock();

    return ret;
}

void KlcKo_RegOps(void *ppops, int ops_size)
{
    struct bpf_verifier_ops **ops = ppops;

    g_klc_old_get_func_proto = (void*)(*ops)->get_func_proto;

    memcpy(&g_klc_hook_ops, *ops, ops_size);
    *ops = &g_klc_hook_ops;

    g_klc_hook_ops.get_func_proto = (void*)_klcko_hook_get_func_proto;
}
EXPORT_SYMBOL(KlcKo_RegOps);

void KlcKo_SetExtFunc(void *func)
{
    g_klc_extern_func = func;

    if (! func) {
        synchronize_rcu();
    }
}
EXPORT_SYMBOL(KlcKo_SetExtFunc);

static int __init klc_base_init(void)
{
    try_module_get(THIS_MODULE);
    return 0;
}

static void __exit klc_base_exit(void)
{
}

module_init(klc_base_init)
module_exit(klc_base_exit)
MODULE_LICENSE("GPL");

