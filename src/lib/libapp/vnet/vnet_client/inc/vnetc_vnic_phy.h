#ifndef __VNETC_VNIC_PHY_H_
#define __VNETC_VNIC_PHY_H_

#include "utl/net.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

extern MAC_ADDR_S * VNET_VNIC_PHY_GetVnicMac ();
extern UINT VNETC_VNIC_PHY_GetVnicIfIndex ();
extern BS_STATUS VNET_VNIC_PHY_Init ();
extern BS_STATUS VNET_VNIC_CreateVnic ();
extern VOID VNET_VNIC_DelVnic ();
extern VOID VNET_VNIC_PHY_SetMediaStatus(IN UINT uiStatus);
extern BS_STATUS VNETC_VNIC_PHY_GetIp(OUT UINT *puiIp, OUT UINT *puiMask);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETC_VNIC_PHY_H_*/

