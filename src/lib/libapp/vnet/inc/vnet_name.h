/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-9-5
* Description: 
* History:     
******************************************************************************/

#ifndef __VNET_NAME_H_
#define __VNET_NAME_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#pragma pack(1)

typedef struct
{
    USHORT usVer;
    USHORT usType;
}VNET_NAME_HEAD_S;

typedef struct
{
    VNET_NAME_HEAD_S stHead;
    CHAR szNetName[VNET_CONF_MAX_NET_NAME_LEN + 1];
}VNET_NAME_NOTIFY_S;

#pragma pack()


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNET_NAME_H_*/


