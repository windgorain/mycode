/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-12-10
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
#include "comp/comp_ldap.h"

#include "ldapapp_inner.h"


BS_STATUS LDAPAPP_Init()
{
    LDAPAPP_Lock_Init();
    LDAPAPP_Cmd_Init();
    LDAPAPP_Schema_Init();

    return BS_OK;
}

