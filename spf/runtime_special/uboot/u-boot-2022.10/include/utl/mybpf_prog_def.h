/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
********************************************************/
#ifndef _MYBPF_PROG_DEF_H
#define _MYBPF_PROG_DEF_H
#ifdef __cplusplus
extern "C"
{
#endif


typedef struct {
    const void **base_helpers;
    const void **sys_helpers;
    const void **user_helpers;
    const void **tmp_helpers;
    void *maps; 
    void **global_map_data;
    void *loader_node;
}MYBPF_AOT_PROG_CTX_S;

typedef struct {
    RCU_NODE_S rcu_node;
    char sec_name[128];
    char prog_name[64];
    void *loader_node;
    UINT attached; 
    int insn_len; 
    void *insn; 
}MYBPF_PROG_NODE_S;

#ifdef __cplusplus
}
#endif
#endif 
