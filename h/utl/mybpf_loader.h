/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _MYBPF_LOADER_H
#define _MYBPF_LOADER_H

#include "utl/mybpf_runtime.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MYBPF_LOADER_FLAG_AUTO_ATTACH 0x1  /* load时是否进行自动attach */
#define MYBPF_LOADER_FLAG_KEEP_MAP    0x2  /* replace时是否保留原来的map */

typedef struct {
    char *instance; /* instance name */
    char *filename; /* object file name */
    char *sec_name; /* sec name to load. if null then all sec */
    char *func_name;/* func name to load. if null then all func */
    UINT flag;
}MYBPF_LOADER_PARAM_S;

enum {
    MYBPF_ATTACH_TYPE_XDP = 0,
};

#define MYBPF_LOADER_MAX_MAPS 32
#define MYBPF_LOADER_MAX_PROGS 32

typedef struct {
    MYBPF_LOADER_PARAM_S param;
    MAP_LINK_NODE_S link_node;
    int prog_count;
    int prog_fd[MYBPF_LOADER_MAX_PROGS];
    int map_count;
    int map_def_size;
    int map_fd[MYBPF_LOADER_MAX_MAPS];
}MYBPF_LOADER_NODE_S;

int MYBPF_LoaderLoad(MYBPF_RUNTIME_S *runtime, MYBPF_LOADER_PARAM_S *p);
int MYBPF_AttachAuto(MYBPF_RUNTIME_S *runtime, char *instance);
void MYBPF_LoaderUnload(MYBPF_RUNTIME_S *runtime, char *instance);
MYBPF_LOADER_NODE_S * MYBPF_LoaderGet(MYBPF_RUNTIME_S *runtime, char *instance);
MYBPF_LOADER_NODE_S * MYBPF_LoaderGetNext(MYBPF_RUNTIME_S *runtime, INOUT void **iter);

#ifdef __cplusplus
}
#endif
#endif //MYBPF_LOADER_H_
