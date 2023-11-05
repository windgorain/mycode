/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "klcko_impl.h"
#include "utl/mdef_utl.h"
#include "klc/xfunc/xfunc_klc.h"

#define MDEF_DEF_FUNC(str,id) void XFUNC_Func##str(KLC_XFUNC_S *state, u64 p1, u64 p2) {}
    MDEF_1000_FUNCS
#undef MDEF_DEF_FUNC

#define MDEF_DEF_FUNC(str,id) XFUNC_Func##str,
PF_XFUNC_FUNC g_klcko_xfuncs[] = {
    MDEF_1000_FUNCS
};
#undef MDEF_DEF_FUNC

