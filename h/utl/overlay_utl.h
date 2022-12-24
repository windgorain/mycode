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


/* 解析报文真正用到的是内层报文，所以把外层头放在overlay head中，pkt中的指针指向内层信息 */
typedef struct {
    int type;   /*overlay 类型*/
    IP_HEAD_S  *outer_ip_header;    /* 外层ip头 */
    UDP_HEAD_S *outer_udp_header;   /* 外层udp头 */
    void *header;   /*overlay header, 与type对应*/
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
#endif //OVERLAY_H_
