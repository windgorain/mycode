/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _ACL_APP_FUNC_H
#define _ACL_APP_FUNC_H
#ifdef __cplusplus
extern "C"
{
#endif

void AclApp_CompInit();

BS_STATUS AclAppIP_Init();
void AclAppIP_DestroyMuc(ACL_MUC_S *acl_muc);

BS_STATUS AclAppURL_Init();
BS_STATUS AclAppIP_Save(IN HANDLE hFile);
BS_STATUS AclAppURL_Save(IN HANDLE hFile);
BS_STATUS AclDomain_Save(IN HANDLE hFile);

BS_STATUS AclAppIp_Ioctl(int muc_id, COMP_ACL_IOCTL_E enCmd, IN VOID *pData);
BS_ACTION_E AclAppIp_Match(int muc_id, UINT ulListID, IN IPACL_MATCH_INFO_S *pstMatchInfo);

void* AclDomainGroup_FindByName(IN INT muc_id, IN CHAR* pcName);
int AclDomainGroup_Init(void);
BS_STATUS AclDomainGroup_Save(IN HANDLE hFile);
BS_ACTION_E AclDomain_Match(int muc_id, UINT ulListID, IN DOMAINACL_MATCH_INFO_S *pstMatchInfo);
BS_ACTION_E AclURL_Match(int muc_id, UINT ulListID, IN URL_ACL_MATCH_INFO_S *pstMatchInfo);
BS_STATUS AclDomain_Ioctl(IN INT muc_id, IN COMP_ACL_IOCTL_E enCmd, IN VOID *pData);
BS_STATUS ACL_URL_Ioctl(IN INT muc_id, IN COMP_ACL_IOCTL_E enCmd, IN VOID *pData);

#ifdef __cplusplus
}
#endif
#endif 
