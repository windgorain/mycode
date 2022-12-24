/******************************************************************************
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "comp/comp_acl.h"
#include "../h/acl_muc.h"
#include "../h/acl_app_func.h"


PLUG_API BS_ACTION_E ACL_Match(int muc_id, ACL_TYPEL_E enType, UINT ulListID, IN VOID *pstMatchInfo)
{
    BS_ACTION_E eRet = BS_ACTION_UNDEF;
    
    switch (enType) {
        case ACL_TYPE_IP:
            eRet = AclAppIp_Match(muc_id, ulListID, pstMatchInfo);
            break;
        case ACL_TYPE_DOMAIN:
            eRet = AclDomain_Match(muc_id, ulListID, pstMatchInfo);
            break;
		case ACL_TYPE_URL:
			eRet = AclURL_Match(muc_id, ulListID, pstMatchInfo);
        default:
            eRet = BS_ACTION_UNDEF;
            break;
    }

    return eRet;
}

PLUG_API BS_STATUS ACL_Ioctl(int muc_id, ACL_TYPEL_E enType, COMP_ACL_IOCTL_E enCmd, IN VOID *pData)
{
    BS_STATUS eRet = BS_OK;

    switch (enType) {
        case ACL_TYPE_IP:
            eRet = AclAppIp_Ioctl(muc_id, enCmd, pData);
            break;
        case ACL_TYPE_URL:
			eRet = ACL_URL_Ioctl(muc_id, enCmd, pData);
            break;
        case ACL_TYPE_DOMAIN:
			eRet = AclDomain_Ioctl(muc_id, enCmd, pData);
			break;
        default:
            eRet = BS_NOT_SUPPORT;
            break;
    }

    return eRet;
}
