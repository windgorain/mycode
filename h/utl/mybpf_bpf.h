/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _MYBPF_BPF_H
#define _MYBPF_BPF_H
#include "klc/klc_def.h"
#ifdef __cplusplus
extern "C"
{
#endif

/* 运行原始code */
U64 MYBPF_RunBpfCode(void *code, U64 r1, U64 r2, U64 r3, U64 r4, U64 r5);
/* 运行namefunc之类的自定义code */
U64 MYBPF_RunKlcCode(KLC_BPF_HEADER_S *klc_code, U64 r1, U64 r2, U64 r3, void *ctx);

#ifdef __cplusplus
}
#endif
#endif //MYBPF_BPF_H_
