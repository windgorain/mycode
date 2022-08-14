/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include <math.h>
#include "utl/wasm_utl.h"
#include "wasm_def.h"
#include "wasm_func.h"

static void _wasm_parse_args(WASM_MODULE_S *m, WASM_TYPE_S *type, int argc, char **argv)
{
    int i;
    WASM_STACKVALUE_S v;

    for (i = 0; i < argc; i++) {
        TXT_Lower(argv[i]);
        v.value_type = type->params[i];

        switch (type->params[i]) {
            case WASM_I32:
                v.value.uint32 = strtoul(argv[i], NULL, 0);
                break;
            case WASM_I64:
                v.value.uint64 = strtoull(argv[i], NULL, 0);
                break;
            case WASM_F32:
                if (strncmp("-nan", argv[i], 4) == 0) {
                    v.value.f32 = -NAN;
                } else {
                    v.value.f32 = (float) strtod(argv[i], NULL);
                }
                break;
            case WASM_F64:
                if (strncmp("-nan", argv[i], 4) == 0) {
                    v.value.f64 = -NAN;
                } else {
                    v.value.f64 = strtod(argv[i], NULL);
                }
                break;
        }

        WASM_StackPush(m, &v);
    }
}

BOOL_T WASM_RunLine(WASM_MODULE_S *m, char *line)
{
    int argc;
    char *argv[32];

    argc = TXT_StrToToken(line, " \t", argv, 32);

    if (argc == 0) {
        return FALSE;
    }

    WASM_BLOCK_S *func = WASM_GetExport(m, argv[0]);

    if (!func) {
        BS_PRINT_ERR("no exported function named '%s'\n", argv[0]);
        return FALSE;
    }

    if (func->type->param_count >= argc) {
        BS_PRINT_ERR("no enough params \n");
        return FALSE;
    }

    _wasm_parse_args(m, func->type, argc - 1, argv + 1);

    return WASM_Run(m, func->fidx);
}

