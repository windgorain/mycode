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

enum {
    MYBPF_HP_TCMD = 0, /* trigger cmd */
    MYBPF_HP_XDP,

    MYBPF_HP_MAX
};

typedef struct {
    MAP_HANDLE loader_map;
    UFD_S *ufd_ctx;

    DLL_HEAD_S hp_list[MYBPF_HP_MAX];
}MYBPF_RUNTIME_S;

int MYBPF_RuntimeInit(OUT MYBPF_RUNTIME_S *runtime, UINT ufd_capacity);
void MYBPF_RuntimeFini(OUT MYBPF_RUNTIME_S *runtime);
BOOL_T MYBPF_RuntimeIsInited(MYBPF_RUNTIME_S *runtime);

MYBPF_RUNTIME_S * MYBPF_GetDftRuntime(void);

#ifdef __cplusplus
}
#endif
#endif //MYBPF_RUNTIME_H_
