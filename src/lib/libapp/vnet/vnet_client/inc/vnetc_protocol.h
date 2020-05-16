
#ifndef __VNETC_PROTOCOL_H_
#define __VNETC_PROTOCOL_H_

#include "utl/mbuf_utl.h"
#include "utl/mime_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef struct
{
    UINT uiTpID;
    MBUF_S *pstMBuf;
}VNETC_PROTOCOL_PACKET_INFO_S;

typedef BS_STATUS (*PF_VNETC_PROTOCOL_DO_FUNC)(IN MIME_HANDLE hMime, IN VNETC_PROTOCOL_PACKET_INFO_S *pstPacketInfo);

BS_STATUS VNETC_Protocol_Init();

BS_STATUS VNETC_Protocol_SendData
(
    IN UINT uiTpID,
    IN VOID *pData,
    IN UINT ulDataLen
);

BS_STATUS VNETC_Auth_Input(IN MIME_HANDLE hMime, IN VNETC_PROTOCOL_PACKET_INFO_S *pstPacketInfo);
BS_STATUS VNETC_PeerInfo_Input(IN MIME_HANDLE hMime, IN VNETC_PROTOCOL_PACKET_INFO_S *pstPacketInfo);
BS_STATUS VNETC_EnterDomain_Input(IN MIME_HANDLE hMime, IN VNETC_PROTOCOL_PACKET_INFO_S *pstPacketInfo);
BS_STATUS VNETC_Logout_PktInput(IN MIME_HANDLE hMime, IN VNETC_PROTOCOL_PACKET_INFO_S *pstPacketInfo);
BS_STATUS VNETC_Protocol_Input(IN UINT uiTpId, IN MBUF_S *pstMbuf);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETC_PROTOCOL_H_*/

