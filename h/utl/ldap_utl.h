/******************************************************************************
* Copyright (C), 2000-2006,  Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2017-1-10
* Description: 
* History:     
******************************************************************************/

#ifndef __LDAP_UTL_H_
#define __LDAP_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef HANDLE LDAPUTL_HANDLE;

LDAPUTL_HANDLE LDAPUTL_Create
(
    IN CHAR *pcServerAddress,
    IN USHORT usServerPort,
    IN CHAR *pcUserName,
    IN CHAR *pcPassword
);
VOID LDAPUTL_Destroy(IN LDAPUTL_HANDLE hLdapNode);
BS_STATUS LDAPUTL_StartAuth(IN LDAPUTL_HANDLE hLdapNode);
INT LDAPUTL_GetSocketID(IN LDAPUTL_HANDLE hLdapNode);
/*
   返回值: BS_OK:成功. BS_CONTINUE:还没完成,需要继续. 其他:出错
*/
BS_STATUS LDAPUTL_Run(IN LDAPUTL_HANDLE hLdapNode);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__LDAP_UTL_H_*/


