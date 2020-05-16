
#ifndef __VNETS_UDP_SERVICE_H_
#define __VNETS_UDP_SERVICE_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define VNETS_UDP_SERVICE_FLAG_CUT_HEAD 0x1 /* cut掉IP和UDP头 */

typedef struct
{
    UINT uiLocalIP;
    UINT uiPeerIP;
    USHORT usLocalPort;
    USHORT usPeerPort;
}VNETS_UDP_SERVICE_PARAM_S;

typedef BS_STATUS (*PF_VNETS_SERVICE_FUNC)(IN MBUF_S *pstMbuf, IN VNETS_UDP_SERVICE_PARAM_S *pstParam);

VOID VNETS_UDP_Service_RegService
(
    IN USHORT usPort/* 网络序 */,
    IN UINT uiFlag, /* VNETS_UDP_SERVICE_FLAG_XXX */
    IN PF_VNETS_SERVICE_FUNC pfServiceFunc
);
BS_STATUS VNETS_UDP_Service_Input(IN MBUF_S *pstMbuf);
BS_STATUS VNETS_UDP_Service_Output(IN MBUF_S *pstMbuf, IN VNETS_UDP_SERVICE_PARAM_S *pstParam);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETS_UDP_SERVICE_H_*/

