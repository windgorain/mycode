/******************************************************************************
* Copyright (C), 2000-2006,  Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2017-5-5
* Description: 
* History:     
******************************************************************************/

#ifndef __VNIC_FUNC_H_
#define __VNIC_FUNC_H_

#include "comp/comp_vnic.h"

#ifdef __cplusplus
    extern "C" {
#endif 

BS_STATUS VnicIns_Init();
BS_STATUS VnicIns_Start();
BS_STATUS VnicIns_Stop();
BS_STATUS VnicIns_RegRecver(IN PF_VNICAPP_PKT_RECEIVER pfRecver);
VNIC_HANDLE VnicIns_GetVnicHandle();
BS_STATUS VnicIns_Output(IN MBUF_S *pstMbuf);
MAC_ADDR_S * VnicIns_GetVnicMac();

#ifdef __cplusplus
    }
#endif 

#endif 


