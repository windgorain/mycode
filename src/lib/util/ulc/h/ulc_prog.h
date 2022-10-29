/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _ULC_PROG_H
#define _ULC_PROG_H
#include "ulc_osbase.h"
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    char prog_name[64];
    UINT attached; /* attached位,每位标志一个attach点 */
    int used_map_cnt; 
    int used_maps[MAX_USED_MAPS];
    int insn_len; /* insn 字节数 */
    struct bpf_insn insn[0];
}ULC_PROG_NODE_S;

ULC_PROG_NODE_S * ULC_PROG_Alloc(struct bpf_insn *insn, int len /* insn的字节数 */, char *prog_name);
void ULC_PROG_Free(ULC_PROG_NODE_S *prog);
int ULC_PROG_Add(ULC_PROG_NODE_S *prog);
ULC_PROG_NODE_S * ULC_PROG_GetByFD(int fd);
ULC_PROG_NODE_S * ULC_PROG_RefByFD(int fd);
void ULC_PROG_Close(int fd);

#ifdef __cplusplus
}
#endif
#endif 
