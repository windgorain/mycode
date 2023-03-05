/*================================================================
*   Created by LiXingang, Copyright LiXingang
*   Description: 
*
================================================================*/
#ifndef _MYBPF_PROG_H
#define _MYBPF_PROG_H

#include "utl/mybpf_utl.h"
#include "utl/mybpf_runtime.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MYBPF_PROG_MAX_MAPS	64

typedef struct {
    RCU_NODE_S rcu_node;
    char sec_name[128];
    char prog_name[64];
    void *loader_node;
    UINT attached; /* attached位,每位标志一个attach点 */
    int fd;
    int insn_len; /* insn 字节数 */
    void *insn;
}MYBPF_PROG_NODE_S;

typedef struct xdp_buff {
	void *data;
	void *data_end;
	void *data_meta;
	/* Below access go through struct xdp_rxq_info */
	UINT ingress_ifindex; 
	UINT rx_queue_index; 
}MYBPF_XDP_BUFF_S;

MYBPF_PROG_NODE_S * MYBPF_PROG_Alloc(void *insn, int len, char *sec_name, char *prog_name);
void MYBPF_PROG_Free(MYBPF_RUNTIME_S *runtime, MYBPF_PROG_NODE_S *prog);
int MYBPF_PROG_Add(MYBPF_RUNTIME_S *runtime, MYBPF_PROG_NODE_S *prog);
MYBPF_PROG_NODE_S * MYBPF_PROG_GetByFD(MYBPF_RUNTIME_S *runtime, int fd);
void MYBPF_PROG_Close(MYBPF_RUNTIME_S *runtime, int fd);
void MYBPF_PROG_ShowProg(MYBPF_RUNTIME_S *runtime, PF_PRINT_FUNC print_func);
int MYBPF_PROG_GetByFuncName(MYBPF_RUNTIME_S *runtime, char *instance, char *name);
int MYBPF_PROG_GetBySecName(MYBPF_RUNTIME_S *runtime, char *instance, char *sec_name);
MYBPF_PROG_NODE_S * MYBPF_PROG_GetNext(MYBPF_RUNTIME_S *runtime, char *instance, char *sec_prefix,
        MYBPF_PROG_NODE_S *current);
int MYBPF_PROG_Run(MYBPF_PROG_NODE_S *prog, OUT UINT64 *bpf_ret, UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5);
int MYBPF_PROG_RunByFd(MYBPF_RUNTIME_S *runtime, int fd, OUT UINT64 *bpf_ret,
        UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5);

int MYBPF_PROG_ReplaceMapFdWithMapPtr(MYBPF_RUNTIME_S *runtime, int *map_fds, MYBPF_LOADER_NODE_S *n);
int MYBPF_PROG_FixupExtCalls(void *insts, int len);

#ifdef __cplusplus
}
#endif
#endif //MYBPF_PROG_H_
