/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _ULC_BPF_H
#define _ULC_BPF_H
#ifdef __cplusplus
extern "C"
{
#endif

U64 ULC_RunBpfCode(void *data, U64 r1, U64 r2, U64 r3, U64 r4, U64 r5);
U64 ULC_RunKlcCode(void *data, U64 r1, U64 r2, U64 r3, void *ctx);

#ifdef __cplusplus
}
#endif
#endif //ULC_BPF_H_
