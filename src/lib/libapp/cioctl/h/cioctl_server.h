/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _CIOCTL_SERVER_H
#define _CIOCTL_SERVER_H
#ifdef __cplusplus
extern "C"
{
#endif

int CIOCTL_SERVER_SetNamePKey(char *pkey_name);
int CIOCTL_SERVER_SetNameString(char *name);
int CIOCTL_SERVER_SetNameDefault();
int CIOCTL_SERVER_GetNameType();
char * CIOCTL_SERVER_GetName();
BOOL_T CIOCTL_SERVER_IsEnabled();
int CIOCTL_SERVER_Enable();

#ifdef __cplusplus
}
#endif
#endif 
