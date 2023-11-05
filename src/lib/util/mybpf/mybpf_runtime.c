/*================================================================
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2017-10-1
* Description: 
*
================================================================*/
#include "bs.h"
#include "utl/map_utl.h"
#include "utl/mybpf_runtime.h"
#include "utl/mybpf_loader.h"

int MYBPF_RuntimeInit(OUT MYBPF_RUNTIME_S *runtime)
{
    memset(runtime, 0, sizeof(*runtime));

    runtime->loader_map = MAP_ListCreate(NULL);
    if (! runtime->loader_map) {
        RETURN(BS_ERR);
    }

    int i;
    for (i=0; i<MYBPF_HP_MAX; i++) {
        DLL_INIT(&runtime->hp_list[i]);
    }

    return 0;
}

void MYBPF_RuntimeFini(OUT MYBPF_RUNTIME_S *runtime)
{
    MYBPF_LoaderUnloadAll(runtime);
    MAP_Destroy(runtime->loader_map, NULL, NULL);
    runtime->loader_map = NULL;
}

BOOL_T MYBPF_RuntimeIsInited(MYBPF_RUNTIME_S *runtime)
{
    if (runtime->loader_map) {
        return TRUE;
    }
    return FALSE;
}
