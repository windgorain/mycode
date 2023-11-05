/*****************************************************************************
*   Created by LiXingang, Copyright LiXingang
*   Description: 
*
*****************************************************************************/
#ifndef _ACL_PORT_GROUP_H
#define _ACL_PORT_GROUP_H
#ifdef __cplusplus
extern "C"
{
#endif

int AclPortGroup_Init(void);
int AclPortGroup_Save(HANDLE hFile);
void * AclPortGroup_FindByName(int muc_id, char *list_name);

#ifdef __cplusplus
}
#endif
#endif 
