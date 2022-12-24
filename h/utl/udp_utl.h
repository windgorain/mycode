
#ifndef __UDP_UTL_H_
#define __UDP_UTL_H_

#include "net.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

/* UDP header*/
typedef struct 
{
	USHORT usSrcPort;		// Source port
	USHORT usDstPort; 		// Destination port
	USHORT usDataLength; 	/* Datagram length. 包含udp首部和用户数据*/
	USHORT usCrc;			// Checksum
}UDP_HEAD_S;

USHORT UDP_CheckSum
(
    IN UCHAR *pucBuf,
	IN UINT ulLen,
	IN UCHAR *pucSrcIp,
	IN UCHAR *pucDstIp
);

UDP_HEAD_S * UDP_GetUDPHeader(IN VOID *pucData, IN UINT uiDataLen, IN NET_PKT_TYPE_E enPktType);

CHAR * UDP_Header2String(IN VOID *udp, OUT CHAR *info, IN UINT infosize);

/* 检查UDP头长度是否足够 */
static inline BOOL_T UDP_IsHeaderEnough(UDP_HEAD_S *udp_head, int len)
{
    if (len < sizeof(UDP_HEAD_S)) {
        return FALSE;
    }

    return TRUE;
}

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__UDP_UTL_H_*/


