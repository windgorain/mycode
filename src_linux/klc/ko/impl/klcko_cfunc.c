/*================================================================
*   Created by LiXingang
*   Copyright: LiXingang
*   Description: call back func
*
================================================================*/
#include "klcko_impl.h"
#include "utl/mdef_utl.h"

static u64 _klcko_cfunc_do(char *func_name, KLC_CFUNC_PARAM_S *p)
{
    return KlcKo_NameLoadRun(func_name, p, 0, 0);
}

static char g_klcko_cfunc0_names[1000][KLC_FUNC_NAME_SIZE];
static char g_klcko_cfunc1_names[1000][KLC_FUNC_NAME_SIZE];
static char g_klcko_cfunc2_names[1000][KLC_FUNC_NAME_SIZE];
static char g_klcko_cfunc3_names[1000][KLC_FUNC_NAME_SIZE];
static char g_klcko_cfunc4_names[1000][KLC_FUNC_NAME_SIZE];
static char g_klcko_cfunc5_names[1000][KLC_FUNC_NAME_SIZE];

#define MDEF_DEF_FUNC(str,id) u64 CFUNC0_Func##str() { \
    KLC_CFUNC_PARAM_S p; \
    p.count = 0; \
    return _klcko_cfunc_do(g_klcko_cfunc0_names[id], &p); \
}
    MDEF_1000_FUNCS
#undef MDEF_DEF_FUNC

#define MDEF_DEF_FUNC(str,id) u64 CFUNC1_Func##str(u64 p1) { \
    KLC_CFUNC_PARAM_S p; \
    p.count = 1; \
    p.p[0] = p1; \
    return _klcko_cfunc_do(g_klcko_cfunc1_names[id], &p); \
}
    MDEF_1000_FUNCS
#undef MDEF_DEF_FUNC

#define MDEF_DEF_FUNC(str,id) u64 CFUNC2_Func##str(u64 p1, u64 p2) { \
    KLC_CFUNC_PARAM_S p; \
    p.count = 2; \
    p.p[0] = p1; \
    p.p[1] = p2; \
    return _klcko_cfunc_do(g_klcko_cfunc2_names[id], &p); \
}
    MDEF_1000_FUNCS
#undef MDEF_DEF_FUNC

#define MDEF_DEF_FUNC(str,id) u64 CFUNC3_Func##str(u64 p1, u64 p2, u64 p3) { \
    KLC_CFUNC_PARAM_S p; \
    p.count = 3; \
    p.p[0] = p1; \
    p.p[1] = p2; \
    p.p[2] = p3; \
    return _klcko_cfunc_do(g_klcko_cfunc3_names[id], &p); \
}
    MDEF_1000_FUNCS
#undef MDEF_DEF_FUNC

#define MDEF_DEF_FUNC(str,id) u64 CFUNC4_Func##str(u64 p1, u64 p2, u64 p3, u64 p4) { \
    KLC_CFUNC_PARAM_S p; \
    p.count = 4; \
    p.p[0] = p1; \
    p.p[1] = p2; \
    p.p[2] = p3; \
    p.p[3] = p4; \
    return _klcko_cfunc_do(g_klcko_cfunc4_names[id], &p); \
}
    MDEF_1000_FUNCS
#undef MDEF_DEF_FUNC

#define MDEF_DEF_FUNC(str,id) u64 CFUNC5_Func##str(u64 p1, u64 p2, u64 p3, u64 p4, u64 p5) { \
    KLC_CFUNC_PARAM_S p; \
    p.count = 5; \
    p.p[0] = p1; \
    p.p[1] = p2; \
    p.p[2] = p3; \
    p.p[3] = p4; \
    p.p[4] = p5; \
    return _klcko_cfunc_do(g_klcko_cfunc5_names[id], &p); \
}
    MDEF_1000_FUNCS
#undef MDEF_DEF_FUNC


