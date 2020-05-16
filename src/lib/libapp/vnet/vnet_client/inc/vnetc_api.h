#ifndef __VNETC_API_H_
#define __VNETC_API_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef CHAR* (*PF_GET_VNET_VERISON)();
typedef BS_STATUS (*PF_SET_SERVER_ADDRESS_FUNC)(IN CHAR *pcServerAddress);
typedef BS_STATUS (*PF_SET_USER_NAME_FUNC)(IN CHAR *pcUserName);
typedef BS_STATUS (*PF_SET_PASSWORD_FUNC)(IN CHAR *pcPassword);
typedef BS_STATUS (*PF_SET_DOMAIN_FUNC)(IN CHAR *pcDomainName);
typedef BS_STATUS (*PF_START_FUNC)();
typedef VNET_USER_STATUS_E (*PF_GET_USER_STATUS_FUNC)();
typedef CHAR* (*PF_VNETC_USER_GetStatusString_FUNC)();
typedef VNET_USER_REASON_E (*PF_GET_USER_REASON_FUNC)();
typedef CHAR* (*PF_VNETC_USER_GetReasonString_FUNC)();
typedef BS_STATUS (*PF_STOP_FUNC)();
typedef BS_STATUS (*PF_VNETC_SetDescription)(IN CHAR *pcDescription);
typedef BS_STATUS (*PF_VNETC_ADDR_MONITOR_REG_NOTIFY)
(
    IN PF_VNETC_AddrMonitor_Notify_Func pfFunc,
    IN USER_HANDLE_S *pstUserHandle
);
typedef UINT (*PF_VNETC_AddrMonitor_GetIP)();
typedef UINT (*PF_VNETC_AddrMonitor_GetMask)();

#define VNETC_COMP_NAME "VNETC.API"

typedef struct
{
    COMP_NODE_S comp;
    PF_GET_VNET_VERISON pfGetVersion;
    PF_SET_SERVER_ADDRESS_FUNC pfSetServer;
    PF_SET_USER_NAME_FUNC pfSetUserName;
    PF_SET_PASSWORD_FUNC pfSetPassword;
    PF_SET_DOMAIN_FUNC pfSetDomain;
    PF_VNETC_SetDescription pfSetDescription;
    PF_START_FUNC pfStart;
    PF_STOP_FUNC pfStop;
    PF_GET_USER_STATUS_FUNC pfGetUserStatus;
    PF_VNETC_USER_GetStatusString_FUNC pfGetUserStatusString;
    PF_GET_USER_REASON_FUNC pfGetUserReason;
    PF_VNETC_USER_GetReasonString_FUNC pfGetUserReasonString;
    PF_VNETC_ADDR_MONITOR_REG_NOTIFY pfRegAddrMonitorNotify;
    PF_VNETC_AddrMonitor_GetIP pfAddrMonitorGetIP;
    PF_VNETC_AddrMonitor_GetMask pfAddrMonitorGetMask;
}VNETC_API_TBL_S;

typedef VNETC_API_TBL_S * (*PF_VNETC_LOAD_FUNC)();

BS_STATUS VNETC_API_Init();

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETC_API_H_*/



