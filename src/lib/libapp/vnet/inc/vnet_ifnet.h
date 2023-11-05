/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2011-8-18
* Description: 
* History:     
******************************************************************************/

#ifndef __VNET_IFNET_H_
#define __VNET_IFNET_H_

#ifdef __cplusplus
    extern "C" {
#endif 

typedef enum
{
    VNET_IFNET_TYPE_BLCAK_HOLE = 0,
    VNET_IFNET_TYPE_INLOOP,

    VNET_IFNET_TYPE_L2_UDP,
    VNET_IFNET_TYPE_L2_TCP,
    VNET_IFNET_TYPE_L2_VNIC,

    VNET_IFNET_TYPE_L3_PCAP,

    VNET_IFNET_TYPE_MAX
}VNET_IFNET_TYPE_E;


#ifdef __cplusplus
    }
#endif 

#endif 




