/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "klcko_impl.h"
#include "utl/mdef_utl.h"
#include "klc/xfunc/xfunc_klc.h"

#define MDEF_DEF_FUNC(id) void XFUNC_Func##id(KLC_XFUNC_S *state, u64 p1, u64 p2) {}
    MDEF_FUNCS
#undef MDEF_DEF_FUNC

typedef void (*PF_XFUNC_FUNC)(OUT KLC_XFUNC_S *state, u64 p1, u64 p2);

#define MDEF_DEF_FUNC(id) XFUNC_Func##id,
static PF_XFUNC_FUNC g_klcko_xfuncs[] = {
    MDEF_FUNCS
};
#undef MDEF_DEF_FUNC

static inline u64 klcko_xfuncrun(unsigned int func_id, u64 p1, u64 p2)
{
    KLC_XFUNC_S state = {0};

    if (unlikely(func_id >= ARRAY_SIZE(g_klcko_xfuncs))) {
        KO_Print("impl: func id %u is invalid \n", func_id);
        return KLC_RET_ERR;
    }

    state.ret = KLC_RET_ERR;
    g_klcko_xfuncs[func_id](&state, p1, p2);

    return state.ret;
}

u64 KlcKo_XFuncRun(unsigned int func_id, u64 p1, u64 p2)
{
    return klcko_xfuncrun(func_id, p1, p2);
}

