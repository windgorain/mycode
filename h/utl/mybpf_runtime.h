/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _MYBPF_RUNTIME_H
#define _MYBPF_RUNTIME_H

#include "utl/ufd_utl.h"
#include "utl/map_utl.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    MAP_HANDLE loader_map;
    UFD_S *ufd_ctx;

    DLL_HEAD_S xdp_list;
}MYBPF_RUNTIME_S;

int MYBPF_RuntimeInit(OUT MYBPF_RUNTIME_S *runtime, UINT ufd_capacity);
void MYBPF_RuntimeFini(OUT MYBPF_RUNTIME_S *runtime);

#ifdef __cplusplus
}
#endif
#endif //MYBPF_RUNTIME_H_
