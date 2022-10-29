/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _ULC_BASE_HELPERS_H
#define _ULC_BASE_HELPERS_H
#ifdef __cplusplus
extern "C"
{
#endif

u64 ulc_bpf_call_base(u64 r1, u64 r2, u64 r3, u64 r4, u64 r5);
UINT ULC_BaseHelp_GetFunc(UINT imm);

#ifdef __cplusplus
}
#endif
#endif //ULC_BASE_HELPERS_H_
