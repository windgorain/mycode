/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2013-3-26
* Description: 
* History:     
******************************************************************************/

#ifndef __WAN_VRF_H_
#define __WAN_VRF_H_

#include "utl/vf_utl.h"
#include "app/wan_pub.h"
#include "app/if_pub.h"

#ifdef __cplusplus
    extern "C" {
#endif 

#define WAN_MAX_VRF (32*1024)

#define WAN_VRF_MAX_NAME_LEN 63



#define WAN_VF_INNER_TYPE_PREFIX_CHAR    '.'
#define WAN_VF_INNER_TYPE_PREFIX         "."

typedef enum
{
    WAN_VRF_PROPERTY_INDEX_VRF_INNER_IF = 0,
    WAN_VRF_PROPERTY_INDEX_FIB,
    WAN_VRF_PROPERTY_INDEX_DHCP,
    WAN_VRF_PROPERTY_INDEX_ARP,

    WAN_VRF_PROPERTY_INDEX_MAX
}WAN_VRF_PROPERTY_INDEX_E;

typedef enum {
    WAN_VRF_EVENT_CREATE = 0,
    WAN_VRF_EVENT_DESTROY,
    WAN_VRF_EVENT_TIMER,

    WAN_VRF_EVENT_MAX
}WAN_VRF_EVENT_E;


#define WAN_VRF_TIME 2000 

#define WAN_VRF_TIME_TO_TICK(uiTime) (((uiTime) + WAN_VRF_TIME - 1) / WAN_VRF_TIME)

typedef enum {
    WAN_VRF_REG_PRI_HIGH = 1000,
    WAN_VRF_REG_PRI_NORMAL = 2000,
    WAN_VRF_REG_PRI_LOW = 3000
}WAN_VRF_LISTENER_PRIORITY_E;

BS_STATUS WanVrf_Init();
BS_STATUS WanVrf_Init2();

UINT WanVrf_GetIdByName(IN CHAR *pcName);
BS_STATUS WanVrf_GetNameByID(IN UINT uiVrfID, OUT CHAR szName[WAN_VRF_MAX_NAME_LEN + 1]);

CHAR * WanVrf_GetNameByID2(IN UINT uiVrfID, OUT CHAR szName[WAN_VRF_MAX_NAME_LEN + 1]);
UINT WanVrf_GetNext(IN UINT uiCurrentVrf);

#ifdef __cplusplus
    }
#endif 

#endif 


