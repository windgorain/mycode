/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _ACL_MUC_H
#define _ACL_MUC_H

#include "utl/domain_acl.h"
#include "utl/ip_acl.h"
#include "utl/dpi_acl.h"
#include "utl/domain_group_utl.h"
#include "utl/url_acl.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    RCU_NODE_S rcu_node;
    IPACL_HANDLE ipacl;
    PORT_POOL_HANDLE port_pool_handle;
    ADDRESS_POOL_HANDLE address_pool_handle;
    DPIACL_HANDLE dpiacl_handle;
	URL_ACL_HANDLE urlacl_handle;
	DOMAINACL_HANDLE domainacl_handle;
    DOMAIN_GROUP_HANDLE domain_group_handle;
}ACL_MUC_S;

int AclMuc_Init();
ACL_MUC_S * AclMuc_Get(int muc_id);

#ifdef __cplusplus
}
#endif
#endif 
