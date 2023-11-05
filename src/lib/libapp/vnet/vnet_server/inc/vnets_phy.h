#ifndef __VNETS_PHY_H_
#define __VNETS_PHY_H_

#include "utl/mbuf_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif 

typedef enum
{
    VNETS_PHY_TYPE_UDP,

    VNETS_PHY_TYPE_MAX
}VNETS_PHY_TYPE_E;

typedef struct
{
    UINT uiPeerIp;     
    USHORT usPeerPort; 
}VNETS_UDP_PHY_CONTEXT_S;

typedef union
{
    VNETS_UDP_PHY_CONTEXT_S stUdpPhyContext;
}VNETS_PHY_CONTEXT_U;

typedef struct
{
    UINT uiIfIndex;
    VNETS_PHY_TYPE_E enType;
    VNETS_PHY_CONTEXT_U unPhyContext;
}VNETS_PHY_CONTEXT_S;

#ifdef __cplusplus
    }
#endif 

#endif 

