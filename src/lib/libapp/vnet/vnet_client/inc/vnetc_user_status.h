
#ifndef __VNETC_USER_STATUS_H_
#define __VNETC_USER_STATUS_H_

#include "../../inc/vnet_user_status.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

VOID VNETC_User_SetStatus(IN VNET_USER_STATUS_E enStatus, IN VNET_USER_REASON_E enReason);
VNET_USER_STATUS_E VNETC_User_GetStatus();
VNET_USER_REASON_E VNETC_User_GetReason();
CHAR * VNETC_User_GetStatusString();
CHAR * VNETC_User_GetReasonString();

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETC_USER_STATUS_H_*/


