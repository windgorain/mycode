#ifndef __VNETC_VPN_LINK_H_
#define __VNETC_VPN_LINK_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

BS_STATUS VNETC_VPN_LINK_Input(IN MBUF_S *pstMbuf);
BS_STATUS VNETC_VPN_LINK_Output (IN UINT ulIfIndex, IN MBUF_S *pstMbuf, IN USHORT usProtoType);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETC_VPN_LINK_H_*/




