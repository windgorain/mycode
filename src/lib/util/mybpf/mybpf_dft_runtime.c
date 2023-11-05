/*================================================================
*   Created by LiXingang
*   Description:  默认内置的runtime, 省去自己构建runtime
*
================================================================*/
#include "bs.h"
#include "utl/mybpf_runtime.h"

static MYBPF_RUNTIME_S g_mybpf_dft_runtime;

MYBPF_RUNTIME_S * MYBPF_GetDftRuntime(void)
{
    if (! MYBPF_RuntimeIsInited(&g_mybpf_dft_runtime)) {
        if (MYBPF_RuntimeInit(&g_mybpf_dft_runtime) < 0) {
            return NULL;
        }
    }

    return &g_mybpf_dft_runtime;
}


