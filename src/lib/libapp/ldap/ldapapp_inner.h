/******************************************************************************
* Copyright (C), 2000-2006,  Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2016-12-12
* Description: 
* History:     
******************************************************************************/

#ifndef __LDAPAPP_INNER_H_
#define __LDAPAPP_INNER_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

BS_STATUS LDAPAPP_Cmd_Init();

BS_STATUS LDAPAPP_Lock_Init();
VOID LDAPAPP_Lock();
VOID LDAPAPP_UnLock();


BS_STATUS LDAPAPP_Schema_Init();
BS_STATUS LDAPAPP_Schema_Add(IN CHAR *pcName);
VOID LDAPAPP_Schema_Del(IN CHAR *pcName);
BOOL_T LDAPAPP_Schema_IsExist(IN CHAR *pcName);
BS_STATUS LDAPAPP_Schema_SetDescription(IN CHAR *pcName, IN CHAR *pcDesc);
CHAR * LDAPAPP_Schema_GetDescription(IN CHAR *pcName);
BS_STATUS LDAPAPP_Schema_SetServerAddress(IN CHAR *pcName, IN CHAR *pcAddr, IN CHAR *pcPort);
CHAR * LDAPAPP_Schema_GetServerAddress(IN CHAR *pcName);
CHAR * LDAPAPP_Schema_GetServerPort(IN CHAR *pcName);
CHAR * LDAPAPP_Schema_GetNext(IN CHAR *pcCurrent/* NULL或""表示获取第一个 */);



#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__LDAPAPP_INNER_H_*/


