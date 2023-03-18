/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
********************************************************/
#ifndef _MYBPF_PROG_INNER_H
#define _MYBPF_PROG_INNER_H
#ifdef __cplusplus
extern "C"
{
#endif

/* 运行 prev jit prog时需要的ctx */
typedef struct {
    void *agent_func;
    const void **base_helpers;
    const void **sys_helpers;
    const void **user_helpers;
    UMAP_HEADER_S **maps;
    void **global_map_data;
    void *loader_node;
}MYBPF_PREJIT_PROG_CTX_S;

#ifdef __cplusplus
}
#endif
#endif //MYBPF_PROG_INNER_H_
