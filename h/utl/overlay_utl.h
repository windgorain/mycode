/*================================================================
*   Description：
*
================================================================*/
#ifndef _OVERLAY_H
#define _OVERLAY_H
#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
    OVERLAY_VXLAN,
    OVERLAY_UIS,
    OVERLAY_MAX,
}OVERLAY_TYPE_E;



typedef struct {
    int type;   
    IP_HEAD_S  *outer_ip_header;    
    UDP_HEAD_S *outer_udp_header;   
    void *header;   
}OVERLAY_HEAD_S;

static inline void * OVERLAY_HDR_GET_SIP(OVERLAY_HEAD_S *hdr) {
    if (! hdr->outer_ip_header) {
        return NULL;
    }
    return &hdr->outer_ip_header->unSrcIp.uiIp;
}

static inline void * OVERLAY_HDR_GET_DIP(OVERLAY_HEAD_S *hdr) {
    if (! hdr->outer_ip_header) {
        return NULL;
    }
    return &hdr->outer_ip_header->unDstIp.uiIp;
}

static inline USHORT OVERLAY_HDR_GET_SPORT(OVERLAY_HEAD_S *hdr) {
    if (! hdr->outer_udp_header) {
        return 0;
    }
    return hdr->outer_udp_header->usSrcPort;
}

static inline USHORT OVERLAY_HDR_GET_DPORT(OVERLAY_HEAD_S *hdr) {
    if (! hdr->outer_udp_header) {
        return 0;
    }
    return hdr->outer_udp_header->usDstPort;
}

#ifdef __cplusplus
}
#endif
#endif 
