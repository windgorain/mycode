/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _EBPF_UTL_H
#define _EBPF_UTL_H

#include <linux/netlink.h>
#include <linux/genetlink.h>
#include <linux/if_link.h>
#include <net/if.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    unsigned int map_type;
    unsigned int key_size;
    unsigned int value_size;
    unsigned int max_elem;
    unsigned int flags;
}EBPF_MAP_ATTR_S;

void * EBPF_Open(char *filename);
void EBPF_Close(void *obj);
int EBPF_GetFd(char *pin_filename);
void EBPF_CloseFd(int fd);
int EBPF_Load(void *obj);
int EBPF_GetProgFdByName(void *obj, char *name);
int EBPF_PinProgs(void *obj, char *path);
int EBPF_PinMaps(void *obj, char *path);
int EBPF_Pin(void *obj, char *prog_path, char *map_path);
void EBPF_UnPinProgs(void *obj, char *path);
void EBPF_UnPinMaps(void *obj, char *path);
int EBPF_GetProgFdById(unsigned int bpf_id);
int EBPF_GetMapFdById(unsigned int bpf_id);
int EBPF_GetXdpAttachedFd(char *ifname);
int EBPF_AttachXdp(char *ifname, int fd, UINT flags);
int EBPF_DetachXdp(char *ifname);
int EBPF_GetMapAttr(int fd, OUT EBPF_MAP_ATTR_S *attr);

#ifdef __cplusplus
}
#endif
#endif //EBPF_UTL_H_