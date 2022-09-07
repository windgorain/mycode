/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "klcko_impl.h"
#include "ko/ko_nl.h"

static void *g_klcko_id_funcs[KLC_FUNC_ID_MAX] = {0};
static KUTL_ARRAY_S g_klcko_idarray = {.array_size=KLC_FUNC_ID_MAX, .nodes=g_klcko_id_funcs};

static int _klcko_func_nlget_id_all(NETLINK_MSG_S *msg)
{
    KUTL_NL_GET_S __user *out = msg->reply_ptr;

    if (unlikely(out == NULL)) {
        return KO_ERR_BAD_PARAM;
    }

    return KUtlArray_NlGet(&g_klcko_idarray, out);
}

static int _klcko_func_nlget_id(NETLINK_MSG_S *msg, KLC_FUNC_S *func)
{
    KLC_FUNC_S *p;

    if (unlikely(func->id < 0)) {
        return _klcko_func_nlget_id_all(msg);
    }

    p = KUtlArray_Get(&g_klcko_idarray, func->id);

    return KO_NL_GetOne(msg, p, sizeof(KLC_FUNC_S));
}

static int _klcko_id_func_dump(NETLINK_MSG_S *msg, KLC_FUNC_S *func)
{
    KLC_FUNC_S *p = KUtlArray_Get(&g_klcko_idarray, func->id);
    return KO_NL_Dump(msg, p->insn, p->insn_len);
}

static int _klcko_id_func_add(KLC_FUNC_S *func)
{
    func->module_ptr = KlcKoModule_GetModuleByFullName(func->hdr.name);
    return KUtlArray_Add(&g_klcko_idarray, func->id, (void*)func);
}

static int _klcko_func_del_id(KLC_FUNC_S *func)
{
    if (func->id < 0) {
        KUtlArray_DelAll(&g_klcko_idarray);
        return 0;
    }

    return KUtlArray_Del(&g_klcko_idarray, func->id);
}

static int _klcko_id_func_do(int cmd, NETLINK_MSG_S *msg)
{
    void *data = msg->data;

    switch (cmd) {
        case KLC_NL_FUNC_ADD:
            return _klcko_id_func_add(data);
        case KLC_NL_FUNC_DEL:
            return _klcko_func_del_id(data);
        case KLC_NL_FUNC_GET:
            return _klcko_func_nlget_id(msg, data);
        case KLC_NL_FUNC_DUMP:
            return _klcko_id_func_dump(msg, data);
        default:
            return KO_ERR_BAD_PARAM;
    }
}

static int _klcko_id_func_nlmsg(int cmd, void *msg)
{
    return _klcko_id_func_do(cmd, msg);
}

int KlcKoIDFunc_Init(void)
{
    return KlcKoNl_Reg(KLC_NL_TYPE_ID_FUNC, _klcko_id_func_nlmsg);
}

void KlcKoIDFunc_Fini(void)
{
    KlcKoNl_Reg(KLC_NL_TYPE_ID_FUNC, NULL);
    KUtlArray_DelAll(&g_klcko_idarray);
}

KLC_FUNC_S * KlcKoIDFunc_Get(int id)
{
    return KUtlArray_Get(&g_klcko_idarray, id);
}

void KlcKoIDFunc_DelModule(char *module_prefix)
{
    KUtlArray_DelModule(&g_klcko_idarray, module_prefix);
}

U64 KlcKo_IDLoadRun(U64 key, U64 r1, U64 r2, U64 r3)
{
    U64 ret = KLC_RET_ERR;
    KLC_FUNC_CTX_S ctx = {0};
    KLC_FUNC_S *func;

    rcu_read_lock();

    func = KlcKoIDFunc_Get(key);
    if (likely(func != NULL)) {
        ctx.func = func;
        ret = KlcKo_RunKlcCode(func->insn, r1, r2, r3, &ctx);
    }

    rcu_read_unlock();

    return ret;
}

