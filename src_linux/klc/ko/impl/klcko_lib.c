/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "klcko_impl.h"

#define KLCKO_SYM_MAX 4096

typedef u64  (*PF_KLCHLP_CALL_FUNC)(u64 p, ...);

static void * g_klcko_syms[KLCKO_SYM_MAX];

static inline u64 klcko_run_map_prog(struct bpf_map *progmap, unsigned int index, void *ctx)
{
    struct bpf_array *array = container_of(progmap, struct bpf_array, map);
    struct bpf_prog *prog;

    if (unlikely(index >= array->map.max_entries)) {
        return KLC_RET_ERR;
    }

    prog = READ_ONCE(array->ptrs[index]);
    if (! prog) {
        return KLC_RET_ERR;
    }

    return BPF_PROG_RUN(prog, ctx);
}

static int _klcko_set_sym(unsigned int id, void *sym)
{
    if (unlikely(id >= KLCKO_SYM_MAX)) {
        return -1;
    }

    g_klcko_syms[id] = sym;

    return 0;
}

static void * _klcko_get_sym(unsigned int id)
{
    if (unlikely(id >= KLCKO_SYM_MAX)) {
        return NULL;
    }

    return g_klcko_syms[id];
}

static u64 _klcko_call_sym(unsigned int id, u64 p1, u64 p2, u64 p3)
{
    PF_KLCHLP_CALL_FUNC func;

    if (unlikely(id >= KLCKO_SYM_MAX)) {
        return KLC_RET_ERR;
    }

    func = g_klcko_syms[id];

    if (unlikely(func == NULL)) {
        return KLC_RET_ERR;
    }

    return func(p1, p2, p3);
}

static u64 _klcko_call_sym_ext(unsigned int id, KLC_PARAM_S *p)
{
    PF_KLCHLP_CALL_FUNC func;

    if (unlikely(id >= KLCKO_SYM_MAX)) {
        return KLC_RET_ERR;
    }

    func = g_klcko_syms[id];
    if (unlikely(func == NULL)) {
        return KLC_RET_ERR;
    }

    return func(KLC_PARAM_LISTS(p));
}

static u64 _klcko_sys_call(char *name, u64 p1, u64 p2, u64 p3)
{
    PF_KLCHLP_CALL_FUNC func;

    if (unlikely(name == NULL)) {
        return KLC_RET_ERR;
    }

    func = (void*)kallsyms_lookup_name(name);
    if (unlikely(func == NULL)) {
        return KLC_RET_ERR;
    }

    return func(p1, p2, p3);
}

static u64 _klcko_call_with_param(PF_KLCHLP_CALL_FUNC func, KLC_PARAM_S *p)
{
    if (unlikely(p == NULL)) {
        return func(0);
    }

    return func(KLC_PARAM_LISTS(p));
}

static u64 _klcko_sys_call_ext(char *name, KLC_PARAM_S *param)
{
    PF_KLCHLP_CALL_FUNC func;

    if (unlikely(name == NULL)) {
        return KLC_RET_ERR;
    }

    func = (void*)kallsyms_lookup_name(name);
    if (unlikely(func == NULL)) {
        return KLC_RET_ERR;
    }

    return _klcko_call_with_param(func, param);
}

static u64 _klcko_snprintf(char *buf, int size, char *fmt, IN KLC_PARAM_S *p)
{
    if (unlikely(p == NULL)) {
        return snprintf(buf, size, fmt);
    }

    return snprintf(buf, size, fmt, KLC_PARAM_LISTS(p));
}

static u64 _klcko_printk_string(char *string, u64 p1, u64 p2, u64 p3)
{
    printk_ratelimited(string, p1, p2, p3);
    return 0;
}

static bool _klcko_hook_check_license(struct bpf_prog *prog)
{
    int check_cnt = 2;
    struct bpf_insn *insn;
    int insn_cnt;
    int i;

    insn = KlcKoComp_GetProgInsn(prog);
    insn_cnt = KlcKoComp_GetProgLen(prog);

    if (insn_cnt < check_cnt) {
        return 0;
    }

    for (i=0; i<check_cnt; i++) {
        if ((insn[i].code == 0xb7)
                && (insn[i].dst_reg == 1)
                && (insn[i].src_reg == 0)
                && (insn[i].off == 0)
                && (insn[i].imm == KLCHELP_LICENSE_NUM)) {
            return 1;
        }
    }

    return 0;
}

static int _klcko_handler_pre(struct kprobe *p, struct pt_regs *regs)
{
    struct bpf_verifier_env *env = (void*)PT_REGS_PARM1(regs);
    struct bpf_prog *prog;
    int ops_size;
    struct bpf_verifier_ops **ops;

    if (NULL == KlcKoComp_GetCurrentMm()) {
        return 0;
    }

    if (0 == capable(CAP_SYS_ADMIN)) {
        return 0;
    }

    prog = KlcKoComp_GetProg(env);
    if (prog == NULL) {
        return 0;
    }

    if (! _klcko_hook_check_license(prog)) {
        return 0;
    }

    ops_size = KlcKoComp_GetOpsSize();
    ops = KlcKoComp_GetOps(env);
    if (ops == NULL) {
        return 0;
    }

    KlcKo_RegOps(ops, ops_size);

    return 0;
}

static void _klcko_handler_post(struct kprobe *p, struct pt_regs *regs, unsigned long flags)
{
}
#if 0
static int _klcko_handler_fault(struct kprobe *p, struct pt_regs *regs, int trapnr)
{
    return 0;
}
#endif

static u64 _klcko_load(void *mem, unsigned int count)
{
    u64 ret = 0;
    unsigned char *dst = (void*)&ret;
    unsigned char *src = mem;
    int i;

    if (count > 8) {
        return KLC_RET_ERR;
    }

    for (i=0; i<count; i++) {
        dst[i] = src[i];
    }

    return ret;
}

static struct kprobe g_klc_kp;

static int _klcko_regkp(void)
{
    int ret;

    memset(&g_klc_kp, 0, sizeof(g_klc_kp));
    g_klc_kp.symbol_name = "do_check";
    g_klc_kp.pre_handler = _klcko_handler_pre;
    g_klc_kp.post_handler = _klcko_handler_post;
//    g_klc_kp.fault_handler = _klcko_handler_fault;

    ret = register_kprobe(&g_klc_kp);
    if (ret >= 0) {
        return 0;
    }

    memset(&g_klc_kp, 0, sizeof(g_klc_kp));
    g_klc_kp.symbol_name = "do_check_common";
    g_klc_kp.pre_handler = _klcko_handler_pre;
    g_klc_kp.post_handler = _klcko_handler_post;
//    g_klc_kp.fault_handler = _klcko_handler_fault;

    ret = register_kprobe(&g_klc_kp);
    if (ret < 0) {
        printk(KERN_INFO "register failed, ret=%d\n", ret);
        return ret;
    }

    return 0;
}

int KlcKo_Init(void)
{
    int ret = _klcko_regkp();
    if (ret < 0) {
        return ret;
    }

    KlcKo_SetExtFunc(KlcKo_Do);

    return 0;
}

void KlcKo_Fini(void)
{
    unregister_kprobe(&g_klc_kp);
    KlcKo_SetExtFunc(NULL);
}

static inline u64 _klcko_do_base(u64 cmd, u64 p2, u64 p3, u64 p4, u64 p5)
{
    switch (cmd) {
        case KLCHELP_DO_NOTHING:
            return 0;
        case KLCHELP_KO_VERSION:
            return KLC_KO_VERSION;
        case KLCHELP_KERNEL_VERSION:
            return LINUX_VERSION_CODE;
        case KLCHELP_IS_ENABLE:
            return 1;
        case KLCHELP_SELF:
            return p2;
        case KLCHELP_SET_OSHELPER:
            return KlcKoOsHelper_SetHelper(p2, (void*)p3, p4);
        case KLCHELP_EVENT_PUBLISH:
            return KlcKoEvent_Publish((void*)p2, p3, p4);
        case KLCHELP_EVENT_CTL:
            return KlcKoEvent_Ctl((int)p2, (void*)p3, p4);
        case KLCHELP_GET_NAME_MAP:
            return (unsigned long)(long)KlcKoNameMap_Get((void*)p2);
        case KLCHELP_GET_MODULE_DATA:
            return (unsigned long)(long)KlcKoModule_GetModuleData((void*)p2, p3);
        default:
            return KLC_RET_ERR;
    }
}

static inline u64 _klcko_do_sys(u64 cmd, u64 p2, u64 p3, u64 p4, u64 p5)
{
    switch (cmd) {
        case KLCHELP_GET_CGROUP_CLASSID:
            return KlcKoComp_TaskGetClassID((void*)p2);
        case KLCHELP_KALLSYMS_LOOKUP_NAME:
            return (long)(void*)kallsyms_lookup_name((void*)p2);
        case KLCHELP_SYS_CALL:
            return _klcko_sys_call((void*)p2, p3, p4, p5);
        case KLCHELP_SYS_CALL_EXT:
            return _klcko_sys_call_ext((void*)p2, (void*)p3);
        case KLCHELP_SYM_SET:
            return _klcko_set_sym(p2, (void*)p3);
        case KLCHELP_SYM_GET:
            return (long)_klcko_get_sym(p2);
        case KLCHELP_SYM_CALL:
            return _klcko_call_sym((int)p2, p3, p4, p5);
        case KLCHELP_SYM_CALL_EXT:
            return _klcko_call_sym_ext((int)p2, (void*)p3);
        case KLCHELP_KPRINT_STRING:
            return _klcko_printk_string((void*)p2, p3, p4, p5);
        case KLCHELP_BPF_GET_NEXT_KEY: 
            return ((struct bpf_map*)p2)->ops->map_get_next_key((void*)p2, (void*)p3, (void*)p4);
        case KLCHELP_LOAD:
            return _klcko_load((void*)p2, p3);
        case KLCHELP_GET_NAME_FUNC:
            return (long)KlcKo_GetNameFunc((void*)p2);
        default:
            return KLC_RET_ERR;
    }
}

static inline u64 _klcko_do_run_counted(u64 cmd, u64 p2, u64 p3, u64 p4, u64 p5)
{
    switch (cmd) {
        case KLCHELP_BPF_RUN:
            return KlcKo_RunKlcCode((void*)p2, p3, p4, p5);
        case KLCHELP_ID_LOAD_RUN_BPF:
            return KlcKo_IDLoadRun(p2, p3, p4, p5);
        case KLCHELP_NAME_LOAD_RUN_BPF:
            return KlcKo_NameLoadRun((void*)p2, p3, p4, p5);
        case KLCHELP_XFUNC_RUN:
            return KlcKo_XFuncRun(p2, p3, p4);
        case KLCHELP_XENGINE_RUN:
            return KlcKo_XEngineRun(p2, (void*)(long)p3, p4, p5);
        case KLCHELP_RUN_MAP_PROG:
            return klcko_run_map_prog((void*)p2, p3, (void*)p4);
        case KLCHELP_NAME_LOAD_RUN_BPF_FAST:
            return KlcKo_NameLoadRunFast((void*)p2, p3, p4, p5);
        default:
            return KLC_RET_ERR;
    }
}

static inline u64 _klcko_do_run(u64 cmd, u64 p2, u64 p3, u64 p4, u64 p5)
{
    static DEFINE_PER_CPU(int, call_depth);
    u64 ret;

    /* 调用深度不超过32 */
    if (unlikely(this_cpu_read(call_depth) >= 32)) {
        return KLC_RET_ERR;
    }

    this_cpu_inc(call_depth);
    ret = _klcko_do_run_counted(cmd, p2, p3, p4, p5);
    this_cpu_dec(call_depth);

    return ret;
}

static inline u64 _klcko_do_str(u64 cmd, u64 p2, u64 p3, u64 p4, u64 p5)
{
    switch (cmd) {
        case KLCHELP_STRLEN:
            return strlen((void*)p2);
        case KLCHELP_STRNLEN:
            return strnlen((void*)p2, p3);
        case KLCHELP_STRCHR:
            return (long)strchr((void*)p2, (int)(long)p3);
        case KLCHELP_STRNCHR:
            return (long)strnchr((void*)p2, p3, (int)(long)p4);
        case KLCHELP_STRRCHR:
            return (long)strrchr((void*)p2, (int)(long)p3);
        case KLCHELP_STRSTR:
            return (long)strstr((void*)p2, (void*)p3);
        case KLCHELP_STRNSTR:
            return (long)strnstr((void*)p2, (void*)p3, p4);
        case KLCHELP_SNPRINTF:
            return _klcko_snprintf((void*)p2, p3, (void*)p4, (void*)p5);
        default:
            return KLC_RET_ERR;
    }
}

static inline u64 _klcko_do_mem(u64 cmd, u64 p2, u64 p3, u64 p4, u64 p5)
{
    switch (cmd) {
        case KLCHELP_UNSAFE_MEMCPY:
            return (long)memcpy((void*)p2, (void*)p3, p4);
        case KLCHELP_SAFE_MEMCPY:
            return probe_kernel_read((void*)p2, (void*)p3, p4);
        case KLCHELP_MEMCMP:
            return (long)memcmp((void*)p2, (void*)p3, p4);
        case KLCHELP_MEMCHR:
            return (long)memchr((void*)p2, (int)(long)p3, p4);
        case KLCHELP_MEMMOVE:
            return (long)memmove((void*)p2, (void*)p3, p4);
        default:
            return KLC_RET_ERR;
    }
}

static inline u64 _klcko_do_skb(u64 cmd, u64 p2, u64 p3, u64 p4, u64 p5)
{
    switch(cmd) {
        case KLCHELP_SKB_CONTINUE:
            return pskb_may_pull((void*)p2, p3);
        case KLCHELP_SKB_PUT:
    	    return (unsigned long)skb_put((void*)p2, p3);   
        case KLCHELP_SKB_RESERVE:
            skb_reserve((void*)p2, p3);
			return 0;
        case KLCHELP_SKB_RESET_NETWORK_HEADER:
            skb_reset_network_header((void*)p2);
			return 0;
        case KLCHELP_SKB_RESET_TRANSPORT_HEADER: 
            skb_reset_transport_header((void*)p2);
            return 0;
        case KLCHELP_SKB_SET_TRANSPORT_HEADER:
            skb_set_transport_header((void*)p2, p3);
            return 0; 
        default:
            return KLC_RET_ERR;
    }
}

static inline u64 _klcko_do(u64 cmd, u64 p2, u64 p3, u64 p4, u64 p5)
{
    int class = KLCHELP_CLASS(cmd);

    switch (class) {
        case KLCHELP_CLASS_BASE:
            return _klcko_do_base(cmd, p2, p3, p4, p5);
        case KLCHELP_CLASS_SYS:
            return _klcko_do_sys(cmd, p2, p3, p4, p5);
        case KLCHELP_CLASS_RUN:
            return _klcko_do_run(cmd, p2, p3, p4, p5);
        case KLCHELP_CLASS_STR:
            return _klcko_do_str(cmd, p2, p3, p4, p5);
        case KLCHELP_CLASS_MEM:
            return _klcko_do_mem(cmd, p2, p3, p4, p5);
        case KLCHELP_CLASS_SKB:
            return _klcko_do_skb(cmd, p2, p3, p4, p5);
        default:
            return KLC_RET_ERR;
    }
}   

static inline u64 _klcko_internal_do(u64 cmd, u64 p2, u64 p3, u64 p4, u64 p5)
{
    switch (cmd) {
        case KLC_NAMEDATA_ALLOC:
            return (long)KlcKoNameData_Alloc((void*)p2, p3);
        case KLC_NAMEDATA_FIND_OR_ALLOC:
            return (long)KlcKoNameData_FindOrAlloc((void*)p2, p3);
        case KLC_NAMEDATA_FIND:
            return (long)KlcKoNameData_Find((void*)p2);
        case KLC_NAMEDATA_FIND_KNODE:
            return (long)KlcKoNameData_FindKNode((void*)p2);
        case KLC_NAMEDATA_GET_KNODE_BY_NODE:
            return (long)KlcKoNameData_GetKNodeByNode((void*)p2);
        case KLC_NAMEDATA_FREE_BY_NAME:
            return KlcKoNameData_FreeByName((void*)p2);
        case KLC_NAMEDATA_FREE:
            return KlcKoNameData_Free((void*)p2);
        default:
            return KLC_RET_ERR;
    }

    return KLC_RET_ERR;
}

u64 KlcKo_Do(u64 cmd, u64 p2, u64 p3, u64 p4, u64 p5)
{
    return _klcko_do(cmd, p2, p3, p4, p5);
}

u64 KlcKo_InternalDo(u64 cmd, u64 p2, u64 p3, u64 p4, u64 p5)
{
    return _klcko_internal_do(cmd, p2, p3, p4, p5);
}

