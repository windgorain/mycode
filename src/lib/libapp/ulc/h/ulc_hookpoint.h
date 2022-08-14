/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _ULC_HOOKPOINT_H
#define _ULC_HOOKPOINT_H
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct xdp_buff {
	void *data;
	void *data_end;
	void *data_meta;
	/* Below access go through struct xdp_rxq_info */
	UINT ingress_ifindex; 
	UINT rx_queue_index; 
}ULC_XDP_BUFF_S;

int ULC_HookPoint_Init();
int ULC_HookPoint_XdpAttach(int fd);
int ULC_HookPoint_XdpDetach(int fd);

#ifdef __cplusplus
}
#endif
#endif //ULC_HOOKPOINT_H_
