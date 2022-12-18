/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/map_utl.h"
#include "utl/mybpf_runtime.h"

int MYBPF_RuntimeInit(OUT MYBPF_RUNTIME_S *runtime, UINT ufd_capacity)
{
    memset(runtime, 0, sizeof(*runtime));

    runtime->loader_map = MAP_HashCreate(NULL);
    if (! runtime->loader_map) {
        RETURN(BS_ERR);
    }

    runtime->ufd_ctx = UFD_Create(ufd_capacity);
    if (! runtime->ufd_ctx) {
        MAP_Destroy(runtime->loader_map, NULL, NULL);
        RETURN(BS_ERR);
    }

    DLL_INIT(&runtime->xdp_list);

    return 0;
}


