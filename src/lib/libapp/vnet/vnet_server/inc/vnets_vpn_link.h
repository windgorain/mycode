#ifndef __VNETS_VPN_LINK_H_
#define __VNETS_VPN_LINK_H_

#include "utl/mbuf_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

BS_STATUS VNETS_VPN_LINK_Input(IN MBUF_S *pstMbuf);
BS_STATUS VNETS_VPN_LINK_Output (IN UINT ulIfIndex, IN MBUF_S *pstMbuf, IN USHORT usProtoType);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETS_VPN_LINK_H_*/



