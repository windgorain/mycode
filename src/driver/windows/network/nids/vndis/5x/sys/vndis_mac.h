/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-3-30
* Description: 
* History:     
******************************************************************************/

#ifndef __VNDIS_MAC_H_
#define __VNDIS_MAC_H_

#ifdef __cplusplus
    extern "C" {
#endif 

BOOLEAN VNDIS_MAC_ParseMAC (OUT MACADDR dest, IN char *src);
VOID VNDIS_MAC_GenerateRandomMac (OUT MACADDR mac, IN UCHAR *adapter_name);

#ifdef __cplusplus
    }
#endif 

#endif 


