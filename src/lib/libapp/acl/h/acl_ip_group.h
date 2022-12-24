/*****************************************************************************
*   Created by LiXingang, Copyright LiXingang
*   Description: 
*
*****************************************************************************/
#ifndef _ACL_IP_GROUP_H
#define _ACL_IP_GROUP_H
#ifdef __cplusplus
extern "C"
{
#endif


int AclIPGroup_Init();
int AclIPGroup_Save(HANDLE hFile);
void * AclIPGroup_FindByName(int muc_id, char *list_name);



#ifdef __cplusplus
}
#endif
#endif //ACL_IP_GROUP_H_
