/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "klcko_impl.h"
#include "ko/ko_nl.h"

#define KLCHLP_FUNC_HASH_BUCKETS 1024
#define KLCHLP_FUNC_HASH_MASK (KLCHLP_FUNC_HASH_BUCKETS - 1)

static KUTL_LIST_S g_klcko_namefunc_buckets[KLCHLP_FUNC_HASH_BUCKETS] = {0};
static KUTL_HASH_S g_klcko_namefunc_hash = {
    .bucket_num = KLCHLP_FUNC_HASH_BUCKETS,
    .mask = KLCHLP_FUNC_HASH_MASK,
    .buckets = g_klcko_namefunc_buckets};

static inline int klcko_func_add_name(KLC_FUNC_S *func)
{
    return KUtlHash_Add(&g_klcko_namefunc_hash, &func->hdr);
}

static inline int klcko_func_del_one_name(char *name)
{
    return KUtlHash_Del(&g_klcko_namefunc_hash, name);
}

static inline KLC_FUNC_S * klcko_func_find_name_fast(KUTL_HASH_VAL_S *hash_name)
{
    return (void*)KUtlHash_FindFast(&g_klcko_namefunc_hash, hash_name);
}

static inline KLC_FUNC_S * klcko_func_find_name(char *name)
{
    return (void*)KUtlHash_Find(&g_klcko_namefunc_hash, name);
}

static inline int klcko_func_nlget_name_all(NETLINK_MSG_S *msg)
{
    return KUtlHash_NlGet(&g_klcko_namefunc_hash, msg);
}

static int klcko_func_del_name(KLC_FUNC_S *func)
{
    if (func->hdr.name[0] == '\0')  {
        KUtlHash_DelAll(&g_klcko_namefunc_hash);
        return 0;
    }

    return klcko_func_del_one_name(func->hdr.name);
}

static int klcko_func_nlget_name(NETLINK_MSG_S *msg, KLC_FUNC_S *func)
{
    KLC_FUNC_S *it;

    if (func->hdr.name[0] == '\0') {
        return klcko_func_nlget_name_all(msg);
    }

    it = klcko_func_find_name(func->hdr.name);

    return KO_NL_GetOne(msg, it, sizeof(KLC_FUNC_S));
}

static int klcko_name_func_init(KLC_FUNC_INIT_S *init)
{
    u64 ret;

    if (! init) {
        return KO_ERR_BAD_PARAM;
    }

    ret = KlcKo_NameLoadRun(init->func, 0, 0, 0);
    if (KLC_RET_ERR == ret) {
        return KO_ERR_FAIL;
    }

    return 0;
}

static int klcko_name_func_dump(NETLINK_MSG_S *msg, KLC_FUNC_S *func)
{
    KLC_FUNC_S *it = klcko_func_find_name(func->hdr.name);
    return KO_NL_Dump(msg, it->insn, it->insn_len);
}

static int klcko_name_func_add(KLC_FUNC_S *func)
{
    func->module_ptr = KlcKoModule_GetModuleByFullName(func->hdr.name);
    return klcko_func_add_name(func);
}

static int klcko_name_func_do(int cmd, NETLINK_MSG_S *msg)
{
    int ret = 0;
    void *data = msg->data;

    switch (cmd) {
        case KLC_NL_FUNC_ADD:
            ret = klcko_name_func_add(data);
            break;
        case KLC_NL_FUNC_DEL:
            ret = klcko_func_del_name(data);
            break;
        case KLC_NL_FUNC_GET:
            ret = klcko_func_nlget_name(msg, data);
            break;
        case KLC_NL_FUNC_DUMP:
            ret = klcko_name_func_dump(msg, data);
            break;
        case KLC_NL_FUNC_INIT:
            ret = klcko_name_func_init(data);
            break;
        default:
            return KO_ERR_BAD_PARAM;
            break;
    }

    return ret;
}

static int klcko_func_nlmsg(int cmd, void *msg)
{
    return klcko_name_func_do(cmd, msg);
}

u64 KlcKo_NameLoadRun(char *name, u64 r1, u64 r2, u64 r3)
{
    u64 ret = KLC_RET_ERR;
    KLC_FUNC_S *func;
    KLC_FUNC_CTX_S ctx = {0};

    if (unlikely((!name) || (name[0] == '\0'))) {
        return KLC_RET_ERR;
    }

    rcu_read_lock();
    func = klcko_func_find_name(name);
    if (func) {
        ctx.func = func;
        ret = KlcKo_RunKlcCode(func->insn, r1, r2, r3, &ctx);
    }
    rcu_read_unlock();

    return ret;
}

u64 KlcKo_NameLoadRunFast(KUTL_HASH_VAL_S *hash_name, u64 r1, u64 r2, u64 r3)
{
    u64 ret = KLC_RET_ERR;
    KLC_FUNC_S *func;
    KLC_FUNC_CTX_S ctx = {0};

    if (unlikely(! hash_name)) {
        return KLC_RET_ERR;
    }

    rcu_read_lock();
    func = klcko_func_find_name_fast(hash_name);
    if (func) {
        ctx.func = func;
        ret = KlcKo_RunKlcCode(func->insn, r1, r2, r3, &ctx);
    }
    rcu_read_unlock();

    return ret;
}

void KlcKoNameFunc_DelModule(char *module_prefix)
{
    KUtlHash_DelModule(&g_klcko_namefunc_hash, module_prefix);
}

int KlcKoNameFunc_Init(void)
{
    KlcKoNl_Reg(KLC_NL_TYPE_NAME_FUNC, klcko_func_nlmsg);
    return KUtlHash_Init(&g_klcko_namefunc_hash, NULL);
}

void KlcKoNameFunc_Fini(void)
{
    KlcKoNl_Reg(KLC_NL_TYPE_NAME_FUNC, NULL);
    KUtlHash_DelAll(&g_klcko_namefunc_hash);
}

