/*================================================================
*   Created by LiXingang: 2018.11.15
*   Description: 
*
================================================================*/
#ifndef _FAKECERT_ACL_H
#define _FAKECERT_ACL_H
#ifdef __cplusplus
extern "C"
{
#endif

int fakecert_acl_init(char *acl_file);
int fakecert_acl_is_permit(char *hostname);

#ifdef __cplusplus
}
#endif
#endif //FAKECERT_ACL_H_
