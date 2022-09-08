/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "klcko_impl.h"
#include "utl/mdef_utl.h"
#include "klc/xfunc/xfunc_klc.h"

#define MDEF_DEF_FUNC(str, id) \
        void XEngine_Func##str(KLC_XENGINE_S *engine, u64 p1, u64 p2) {}
    MDEF_1000_FUNCS
#undef MDEF_DEF_FUNC

typedef void (*PF_XENGINE_FUNC)(KLC_XENGINE_S *engine, u64 p1, u64 p2);

#define MDEF_DEF_FUNC(str, id) XEngine_Func##str,
static PF_XENGINE_FUNC g_klcko_xengine_funcs[] = {
    MDEF_1000_FUNCS
};
#undef MDEF_DEF_FUNC

static inline u64 klcko_xenginerun(int start_id, KLC_XENGINE_STATE_S *state, u64 p1, u64 p2)
{
    int err = 0;
    int last = -1;
    unsigned int count = 0;
    KLC_XENGINE_S engine = {0};

    engine.next = start_id;
    engine.iter_limit = KLC_XENGINE_ITER_LIMIT;

    do {
        if (unlikely(engine.next >= ARRAY_SIZE(g_klcko_xengine_funcs))) {
            err = KLC_XENGINE_ERR_ID_ERROR;
            break;
        }

        if (unlikely(count >= KLC_XENGINE_ITER_LIMIT_MAX)) {
            err = KLC_XENGINE_ERR_REACH_MAX_LIMIT;
            break;
        }

        if (unlikely((engine.iter_limit) && (count >= engine.iter_limit))) { 
            err = KLC_XENGINE_ERR_REACH_LIMIT;
            break;
        }

        last = engine.next;
        engine.next = -1;
        engine.ret = KLC_RET_ERR;

        count ++;
        engine.iter_count = count;

        g_klcko_xengine_funcs[last](&engine, p1, p2);

    } while(engine.next >= 0);

    if (NULL != state) {
        memset(state, 0, sizeof(KLC_XENGINE_STATE_S));
        state->iter_count = count;
        state->iter_limit = engine.iter_limit;
        state->start = start_id;
        state->next = engine.next;
        state->last = last;
        state->err = err;
        state->ret = engine.ret;
    }

    if (unlikely(err)) {
        return KLC_RET_ERR;
    }

    return engine.ret;
}

u64 KlcKo_XEngineRun(int start_id, OUT void *state, u64 p1, u64 p2)
{
    return klcko_xenginerun(start_id, state, p1, p2);
}

