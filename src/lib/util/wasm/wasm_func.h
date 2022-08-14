/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _WASM_FUNC_H
#define _WASM_FUNC_H
#ifdef __cplusplus
extern "C"
{
#endif

WASM_TYPE_S *wasm_get_block_type(UCHAR value_type);
void wasm_setup_call(WASM_MODULE_S *m, UINT fidx);
void wasm_push_block(WASM_MODULE_S *m, WASM_BLOCK_S *block, int sp);
BOOL_T wasm_interpret(WASM_MODULE_S *m);
void thunk_out(WASM_MODULE_S *m, UINT fidx);
void init_thunk_in(WASM_MODULE_S *m);
VOID_FUNC setup_thunk_in(uint32_t fidx);

#ifdef __cplusplus
}
#endif
#endif //WASM_FUNC_H_
