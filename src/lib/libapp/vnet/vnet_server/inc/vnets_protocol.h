#ifndef __VNETS_PROTOCOL_H_
#define __VNETS_PROTOCOL_H_

#include "utl/mime_utl.h"
#include "../inc/vnets_context.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef BS_STATUS (*PF_VNETS_PROTOCOL_DO_FUNC)();

typedef struct
{
    UINT uiTpID;
    MBUF_S *pstMBuf;
}VNETS_PROTOCOL_PACKET_INFO_S;

BS_STATUS VNETS_Protocol_Init();

BS_STATUS VNETS_Protocol_Input(IN UINT uiTpID, IN MBUF_S *pstMbuf);

BS_STATUS VNETS_Protocol_SendData
(
    IN UINT uiTpID,
    IN CHAR *pcData,
    IN UINT ulDataLen
);

BS_STATUS VNETS_Auth_Input(IN MIME_HANDLE hMime, IN VNETS_PROTOCOL_PACKET_INFO_S *pstPacketInfo);
BS_STATUS VNETS_PeerInfo_Input(IN MIME_HANDLE hMime, IN VNETS_PROTOCOL_PACKET_INFO_S *pstPacketInfo);
BS_STATUS VNETS_Logout_Recv(IN MIME_HANDLE hMime, IN VNETS_PROTOCOL_PACKET_INFO_S *pstPacketInfo);
BS_STATUS VNETS_EnterDomain_Input(IN MIME_HANDLE hMime, IN VNETS_PROTOCOL_PACKET_INFO_S *pstPacketInfo);
VOID VNETS_Protocol_NoDebugAll();

#ifdef __cplusplus
    }
#endif 

#endif 


