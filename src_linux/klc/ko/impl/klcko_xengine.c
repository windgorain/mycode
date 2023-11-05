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

#define MDEF_DEF_FUNC(str, id) XEngine_Func##str,
PF_XENGINE_FUNC g_klcko_xengine_funcs[] = {
    MDEF_1000_FUNCS
};
#undef MDEF_DEF_FUNC

