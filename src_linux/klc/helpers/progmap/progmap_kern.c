
#include "klc/klc_base.h"
#include "helpers/progmap_klc.h"

#define KLC_MODULE_NAME PROGMAP_KLC_MODULE_NAME
KLC_DEF_MODULE();

SEC_NAME_FUNC(PROGMAP_KLC_WALK_NAME)
int progmap_klc_walk(PROGMAP_PARAM_S *p)
{
    int i;
    int ret = -1;
    int ret1;

    for (i=0; i<p->max_elem; i++) {
        ret1 = KLCHLP_RunMapProg(p->map, i, p->ctx);
        if (ret1 < 0) {
            continue;
        }

        ret = ret1;

        if (p->breaks & (1 << ret)) {
            return ret;
        }
    }

    return ret;
}


