/*================================================================
*   Created by LiXingang, Copyright LiXingang
*   Date: 2017.10.2
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/ufd_utl.h"
#include "utl/umap_utl.h"
#include "utl/mybpf_loader.h"
#include "utl/mybpf_prog.h"
#include "utl/mybpf_insn.h"
#include "utl/bpf_helper_utl.h"
#include "mybpf_osbase.h"
#include "mybpf_def.h"

/* 获取helper函数的offset */
static int _mybpf_prog_get_helper_offset(int imm, void *ud)
{
    PF_BPF_HELPER_FUNC helper_func = BpfHelper_GetFunc(imm);
    if (! helper_func) {
        return 0;
    }

    return helper_func - BpfHelper_BaseHelper;
}


/* fixup extern calls
 * len: 字节数 */
int MYBPF_PROG_FixupExtCalls(void *insts, int len)
{
    return MYBPF_INSN_FixupExtCalls(insts, len, _mybpf_prog_get_helper_offset, NULL);
}

