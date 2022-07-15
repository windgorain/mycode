/*================================================================
*   Created by LiXingang: 2018.11.15
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/hostname_acl.h"

#include "fakecert_acl.h"

static HOSTNAME_ACL_S g_fakecert_acl;

int fakecert_acl_init(char *acl_file)
{
    return HostnameACL_Init(&g_fakecert_acl, acl_file);
}

int fakecert_acl_is_permit(char *hostname)
{
    if (HOSTNAME_ACL_DENY == HostnameACL_Match(&g_fakecert_acl, hostname)) {
        return 0;
    }

    return 1;
}
