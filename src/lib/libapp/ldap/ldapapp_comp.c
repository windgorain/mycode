/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-12-14
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "comp/comp_ldap.h"

#include "ldapapp_inner.h"

static COMP_LDAP_S g_stLdapComp;

VOID LDAPAPP_Comp_Init()
{
    g_stLdapComp.comp.comp_name = COMP_LDAP_NAME;
    COMP_Reg(&g_stLdapComp.comp);
}

