/*================================================================
*   Created by LiXingang, Copyright LiXingang
*   Description: 
*
================================================================*/
#ifndef _CIOCTL_CORE_H
#define _CIOCTL_CORE_H
#ifdef __cplusplus
extern "C"
{
#endif

int CIOCTL_ProcessRequest(CIOCTL_REQUEST_S *req, OUT VBUF_S *reply);

#ifdef __cplusplus
}
#endif
#endif //CIOCTL_CORE_H_
