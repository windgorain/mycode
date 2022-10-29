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

int ULC_HookPoint_XdpAttach(int fd);
int ULC_HookPoint_XdpDetach(int fd);

#ifdef __cplusplus
}
#endif
#endif //ULC_HOOKPOINT_H_
