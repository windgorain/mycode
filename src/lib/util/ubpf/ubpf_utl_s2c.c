/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/ubpf_utl.h"
#include "utl/ubpf/ubpf_int.h"
#include "utl/ubpf/ebpf.h"
#include "pcap.h"

/* cbpf string to cbpf code */
int UBPF_S2c(int linktype, char *cbpf_string, OUT struct bpf_program *bpf_prog)
{
    pcap_t *pcap = NULL;
    int ret;

	pcap = pcap_open_dead(linktype, 65535);
    if(NULL == pcap) {
        RETURN(BS_CAN_NOT_OPEN);
    }
 
    ret = pcap_compile(pcap, bpf_prog, cbpf_string, 0, PCAP_NETMASK_UNKNOWN);
    pcap_close(pcap);

    if (ret < 0) {
        RETURN(BS_ERR);
    }

    return 0;
}

/* cbpf string to ebpf vm */
UBPF_VM_HANDLE UBPF_S2e(int linktype, char *cbpf_string)
{
	struct bpf_program bpf_prog;

    if (0 != UBPF_S2c(linktype, cbpf_string, &bpf_prog)) {
        return NULL;
    }

    return UBPF_C2e(&bpf_prog);
}

int UBPF_S2j(int linktype, char *cbpf_string, OUT UBPF_JIT_S *jit)
{
    UBPF_VM_HANDLE vm = UBPF_S2e(linktype, cbpf_string);

    if (NULL == vm) {
        RETURN(BS_ERR);
    }

    ubpf_jit_fn fn = UBPF_E2j(vm);
    if (NULL == fn) {
        UBPF_Destroy(vm);
        RETURN(BS_ERR);
    }

    jit->vm = vm;
    jit->jit_func = fn;

    return BS_OK;
}

