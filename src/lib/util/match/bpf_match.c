/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/match_utl.h"
#include "utl/ubpf_utl.h"

static BOOL_T bpfmatch_Match(void *pattern_in, void *key_in)
{
    UBPF_JIT_S *pattern = pattern_in;
    BPF_MATCH_KEY_S *key = key_in;

    if (pattern->jit_func) {
        if (pattern->jit_func(key->pkt, key->len)) {
            return TRUE;
        }
    }

    return FALSE;
}

MATCH_HANDLE BpfMatch_Create(UINT max)
{
    return Match_Create(max, sizeof(UBPF_JIT_S), bpfmatch_Match);
}

