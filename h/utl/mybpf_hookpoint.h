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

int MYBPF_XdpAttach(MYBPF_RUNTIME_S *runtime, int fd);
int MYBPF_XdpDetach(MYBPF_RUNTIME_S *runtime, int fd);
int MYBPF_XdpInput(MYBPF_RUNTIME_S *runtime, MYBPF_XDP_BUFF_S *xdp_buf);

#ifdef __cplusplus
}
#endif
#endif //MYBPF_HOOKPOINT_H_
