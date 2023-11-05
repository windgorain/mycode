/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/leb_utl.h"
#include "utl/wasm_utl.h"
#include "wasm_def.h"
#include "wasm_func.h"
#include "wasm_thunk.h"

WASM_MODULE_S * _wa_current_module_;

void thunk_out(WASM_MODULE_S *m, uint32_t fidx)
{
    WASM_BLOCK_S *func = &m->functions[fidx];
    WASM_TYPE_S *type = func->type;

    switch (type->mask) {
        case 0x800          : THUNK_OUT_0(m, func, 0);              break;
        case 0x8001         : THUNK_OUT_1(m, func, 0, i);           break;
        case 0x80011        : THUNK_OUT_2(m, func, 0, i,i);         break;
        case 0x800111       : THUNK_OUT_3(m, func, 0, i,i,i);       break;
        case 0x8001111      : THUNK_OUT_4(m, func, 0, i,i,i,i);     break;
        case 0x810          : THUNK_OUT_0(m, func, i);              break;
        case 0x8101         : THUNK_OUT_1(m, func, i, i);           break;
        case 0x81011        : THUNK_OUT_2(m, func, i, i,i);         break;
        case 0x810111       : THUNK_OUT_3(m, func, i, i,i,i);       break;
        case 0x8101111      : THUNK_OUT_4(m, func, i, i,i,i,i);     break;
        case 0x81011111     : THUNK_OUT_5(m, func, i, i,i,i,i,i);   break;
        case 0x8003         : THUNK_OUT_1(m, func, 0, f);           break;
        case 0x80033        : THUNK_OUT_2(m, func, 0, f,f);         break;
        case 0x800333       : THUNK_OUT_3(m, func, 0, f,f,f);       break;
        case 0x8003333      : THUNK_OUT_4(m, func, 0, f,f,f,f);     break;
        case 0x8303         : THUNK_OUT_1(m, func, f, f);           break;
        case 0x8004         : THUNK_OUT_1(m, func, 0, F);           break;
        case 0x80044        : THUNK_OUT_2(m, func, 0, F,F);         break;
        case 0x800444       : THUNK_OUT_3(m, func, 0, F,F,F);       break;
        case 0x8004444      : THUNK_OUT_4(m, func, 0, F,F,F,F);     break;
        case 0x800444444    : THUNK_OUT_6(m, func, 0, F,F,F,F,F,F); break;
        case 0x8002         : THUNK_OUT_1(m, func, 0, I);           break;
        case 0x8103         : THUNK_OUT_1(m, func, i, f);           break;
        case 0x810121       : THUNK_OUT_3(m, func, i, i,I,i);       break;
        case 0x8404         : THUNK_OUT_1(m, func, F, F);           break;
        case 0x810111112211 : THUNK_OUT_9(m, func, i, i,i,i,i,i,I,I,i,i); break;
        default: BS_WARNNING(("unsupported thunk_out mask 0x%x\n", (unsigned int)type->mask)); break;
    }
}

THUNK_IN_FN_0(m, 0)
THUNK_IN_FN_2(m, 0, i,i)
THUNK_IN_FN_1(m, 0, F)
THUNK_IN_FN_1(m, i, i)
THUNK_IN_FN_2(m, i, i,i)

VOID_FUNC setup_thunk_in(uint32_t fidx)
{
    WASM_MODULE_S *m = _wa_current_module_; 
    WASM_BLOCK_S *func = &m->functions[fidx];
    WASM_TYPE_S *type = func->type;

    m->sp += type->param_count;

    wasm_setup_call(m, fidx);

    for(UINT p=0; p<type->param_count; p++) {
        m->stack[m->fp+p].value_type = type->params[p];
    }

    VOID_FUNC f = NULL;
    switch (type->mask) {
        case 0x800:
            f =  (VOID_FUNC)thunk_in_0_0;
            break;
        case 0x8101:
            f =  (VOID_FUNC)thunk_in_i_i;
            break;
        case 0x80011:
            f =  (VOID_FUNC)thunk_in_0_ii;
            break;
        default:
            BS_WARNNING(("unsupported thunk_in mask 0x%llx\n", type->mask));
            break;
    }

    return f;
}

void WASME_InitThunk(WASM_MODULE_S *m)
{
    _wa_current_module_ = m;
}

