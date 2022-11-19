/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/map_utl.h"
#include "utl/mybpf_runtime.h"

int MYBPF_RuntimeInit(OUT MYBPF_RUNTIME_S *runtime)
{
    memset(runtime, 0, sizeof(*runtime));

    runtime->loader_map = MAP_HashCreate(NULL);
    if (! runtime->loader_map) {
        RETURN(BS_ERR);
    }

    DLL_INIT(&runtime->xdp_list);

    return 0;
}


