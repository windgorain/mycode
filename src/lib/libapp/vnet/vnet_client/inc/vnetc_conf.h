/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-9-5
* Description: 
* History:     
******************************************************************************/

#ifndef __VNETC_CONF_H_
#define __VNETC_CONF_H_

#include "utl/net.h"

#ifdef __cplusplus
    extern "C" {
#endif 

#define VNETC_CONF_SERVER_ADDRESS_MAX_LEN 255

typedef enum
{
    VNETC_CONN_TYPE_UDP = 0,

    VNETC_CONN_TYPE_MAX
}VNETC_CONN_TYPE_E;

BS_STATUS VNETC_SetServer(IN CHAR *pcServerAddress);

CHAR * VNETC_GetServerAddress();

VOID VNETC_SetServerDomain(IN CHAR *pszDomainName);

BS_STATUS VNETC_Start ();

BS_STATUS VNETC_Stop ();

CHAR * VNETC_GetUserName();

CHAR * VNETC_GetUserPasswd();

CHAR * VNETC_GetDomainName();

BS_STATUS VNETC_SetUserName(IN CHAR *pszUserName);

BS_STATUS VNETC_SetUserPasswdSimple(IN CHAR *pszPasswd);

BS_STATUS VNETC_SetUserPasswdCipher(IN CHAR *pszPasswd);

BS_STATUS VNETC_SetDomainName(IN CHAR *pszDomainName);


BS_STATUS VNETC_CONF_ParseServerAddress
(
    IN CHAR *pcServerAddress,
    OUT VNETC_CONN_TYPE_E *penConnType,
    OUT CHAR szServerName[VNETC_CONF_SERVER_ADDRESS_MAX_LEN + 1],
    OUT USHORT *pusPort
);
BS_STATUS VNETC_SetDescription(IN CHAR *pcDescription);
CHAR * VNETC_GetDescription();
CHAR * VNETC_GetVersion();

VOID VNETC_SetC2SConnType(IN VNETC_CONN_TYPE_E enType);
VNETC_CONN_TYPE_E VNETC_GetC2SConnType();

VOID VNETC_CONF_SetSupportDirect(IN BOOL_T bIsSupport);
BOOL_T VNETC_CONF_IsSupportDirect();
VOID VNETC_CONF_SetC2SIfIndex(IN UINT ulIfIndex);
UINT VNETC_CONF_GetC2SIfIndex();
VOID VNETC_SetHostMac(IN MAC_ADDR_S *pstMac);
MAC_ADDR_S * VNETC_GetHostMac();

BS_STATUS VNETC_FuncTbl_Reg();

#ifdef __cplusplus
    }
#endif 

#endif 


