/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2009-3-10
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "../../inc/vnet_conf.h"

#include "../inc/vnetc_rmt.h"
#include "../inc/vnetc_conf.h"
#include "../inc/vnetc_auth.h"

CHAR * VNETC_RMT_GetDomainName()
{
    return VNETC_GetDomainName();
}

CHAR * VNETC_RMT_GetUserName()
{
    return VNETC_GetUserName();
}

CHAR * VNETC_RMT_GetUserPasswd()
{
    return VNETC_GetUserPasswd();
}

int VNETC_RMT_SetUserConfig(U64 p1, U64 p2, U64 p3)
{
    CHAR *pszDomainName = p1;
    CHAR *pszUserName = p2;
    CHAR *pszPasswd = p3;

    BS_STATUS eRet;

    if (BS_OK != (eRet = VNETC_SetDomainName(pszDomainName)))
    {
        return eRet;
    }

    if (BS_OK != (eRet = VNETC_SetUserName(pszUserName)))
    {
        return eRet;
    }
    
    if (BS_OK != (eRet = VNETC_SetUserPasswdSimple(pszPasswd)))
    {
        return eRet;
    }

    return BS_OK;
}

