
#ifndef __VNETC_PHY_H_
#define __VNETC_PHY_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef enum
{
    VNETC_PHY_TYPE_UDP,
    VNETC_PHY_TYPE_VNIC,

    VNETC_PHY_TYPE_MAX
}VNETC_PHY_TYPE_E;

typedef struct
{
    UINT uiPeerIp;     /* 利大會 */
    USHORT usPeerPort; /* 利大會 */
}VNETC_UDP_PHY_CONTEXT_S;

typedef union
{
    VNETC_UDP_PHY_CONTEXT_S stUdpPhyContext;
}VNETC_PHY_CONTEXT_U;

typedef struct
{
    UINT uiIfIndex;
    VNETC_PHY_CONTEXT_U unPhyContext;
}VNETC_PHY_CONTEXT_S;


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETC_PHY_H_*/

