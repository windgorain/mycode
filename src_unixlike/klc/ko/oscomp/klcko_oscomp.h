/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _KLCKO_OSCOMP_H
#define _KLCKO_OSCOMP_H

#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,9,0)
#include <linux/bpf_verifier.h>
#endif

#include "types.h"
#include "ko/ko_utl.h"

#ifdef __cplusplus
extern "C"
{
#endif

void * KlcKoComp_GetCurrentMm(void);
void * KlcKoComp_GetProg(void *env);
void * KlcKoComp_GetProgInsn(struct bpf_prog *prog);
int KlcKoComp_GetProgLen(struct bpf_prog *prog);
void * KlcKoComp_GetOps(void *env);
int KlcKoComp_GetOpsSize(void);
void * KlcKoComp_GetSkbInfo(struct sk_buff *skb, OUT KLC_SKBUFF_INFO_S *skb_info);
int KlcKoComp_GetXdpStructInfo(OUT KLC_XDP_STRUCT_INFO_S *info);
u64 KlcKoComp_TaskGetClassID(void *skb);
int KlcKoComp_MapInc(void *map);
void KlcKoComp_MapDec(void *map);

#ifdef __cplusplus
}
#endif
#endif
