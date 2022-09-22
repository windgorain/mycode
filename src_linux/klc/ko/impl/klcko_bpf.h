/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _KLCKO_BPF_H
#define _KLCKO_BPF_H
#ifdef __cplusplus
extern "C"
{
#endif

typedef u64  (*PF_KLCHLP_ARGS_FUNC)(u64 p1, u64 p2, u64 p3, u64 p4, u64 p5, const struct bpf_insn *);

u64 KlcKo_RunKlcCode(void *data, u64 r1, u64 r2, u64 r3);
u64 KlcKo_RunKlcJitted(void *data, u64 r1, u64 r2, u64 r3);
u64 KlcKo_IDLoadRun(u64 key, u64 r1, u64 r2, u64 r3);
KLC_FUNC_S * KlcKo_GetNameFunc(char *name);
u64 KlcKo_NameLoadRun(char *name, u64 r1, u64 r2, u64 r3);
u64 KlcKo_NameLoadRunFast(KUTL_HASH_VAL_S *hash_name, u64 r1, u64 r2, u64 r3);
u64 KlcKo_XFuncRun(unsigned int func_id, u64 p1, u64 p2);
u64 KlcKo_XEngineRun(int start_id, OUT void *state, u64 p1, u64 p2);
u64 KlcKo_Do(u64 cmd, u64 p2, u64 p3, u64 p4, u64 p5);
u64 KlcKo_InternalDo(u64 cmd, u64 p2, u64 p3, u64 p4, u64 p5);

#ifdef __cplusplus
}
#endif
#endif //KLCKO_BPF_H_
