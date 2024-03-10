/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _MYBPF_HOOKPOINT_H
#define _MYBPF_HOOKPOINT_H
#ifdef __cplusplus
extern "C"
{
#endif

enum {
    MYBPF_HP_RET_CONTINUE = 0,
    MYBPF_HP_RET_STOP
};

typedef struct {
    DLL_NODE_S link_node;
    void *prog;
}MYBPF_HOOKPOINT_NODE_S;

int MYBPF_HookPointAttach(MYBPF_RUNTIME_S *runtime, DLL_HEAD_S *list, MYBPF_PROG_NODE_S *prog);
void MYBPF_HookPointDetach(MYBPF_RUNTIME_S *runtime, DLL_HEAD_S *list, MYBPF_PROG_NODE_S *prog);
int MYBPF_HookPointCall(MYBPF_RUNTIME_S *runtime, int type, MYBPF_PARAM_S *p);

int MYBPF_XdpInput(MYBPF_RUNTIME_S *runtime, MYBPF_XDP_BUFF_S *xdp_buf);

#ifdef __cplusplus
}
#endif
#endif 
