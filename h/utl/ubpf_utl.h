/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _UBPF_UTL_H
#define _UBPF_UTL_H

#include "utl/ubpf/ubpf.h"
#include "pcap.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef void * UBPF_VM_HANDLE;

typedef struct {
    UBPF_VM_HANDLE vm;
    ubpf_jit_fn jit_func;
}UBPF_JIT_S;


/* cbpf string to cbpf code */
int UBPF_S2c(int linktype, char *cbpf_string, OUT struct bpf_program *bpf_prog);
/* cbpf string to ebpf vm */
UBPF_VM_HANDLE UBPF_S2e(int linktype, char *cbpf_string);
int UBPF_S2j(int linktype, char *cbpf_string, OUT UBPF_JIT_S *jit);

/* cbpf to ebpf */
UBPF_VM_HANDLE UBPF_C2e(struct bpf_program *bpf_prog);
/* cbpf to ebpf */
ubpf_jit_fn UBPF_C2j(struct bpf_program *bpf_prog, OUT UBPF_JIT_S *jit);

/* ebpf to jit */
ubpf_jit_fn UBPF_E2j(UBPF_VM_HANDLE vm);

int UBPF_GetJittedSize(UBPF_VM_HANDLE vm);

UBPF_VM_HANDLE UBPF_Create();
void UBPF_Destroy(UBPF_VM_HANDLE vm);
int UBPF_Load(UBPF_VM_HANDLE vm, void *ebpf_code, int ebpf_len);
UBPF_VM_HANDLE UBPF_CreateLoad(void *ebpf_code, int ebpf_len, void **funcs, int funcs_count);
int BPF_Check(void *insts, int num_insts, OUT char * error_msg, int error_msg_size);

#ifdef __cplusplus
}
#endif
#endif //UBPF_UTL_H_
