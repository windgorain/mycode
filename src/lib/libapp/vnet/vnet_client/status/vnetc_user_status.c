
#include "bs.h"

#include "../inc/vnetc_user_status.h"


static VNET_USER_STATUS_E g_enVnetcUserStatus = VNET_USER_STATUS_INIT;
static VNET_USER_REASON_E g_enVnetcUserReason = VNET_USER_REASON_NONE;    


VOID VNETC_User_SetStatus(IN VNET_USER_STATUS_E enStatus, IN VNET_USER_REASON_E enReason)
{
    g_enVnetcUserStatus = enStatus;
    g_enVnetcUserReason = enReason;
}

VNET_USER_STATUS_E VNETC_User_GetStatus()
{
    return g_enVnetcUserStatus;
}

VNET_USER_REASON_E VNETC_User_GetReason()
{
    return g_enVnetcUserStatus;
}

CHAR * VNETC_User_GetStatusString()
{
    static CHAR * apcStatusString[VNET_USER_STATUS_MAX] =
    {
        ""
        "用户不在线",
        "正在连接服务器",
        "正在获取版本号",
        "获取版本号成功",
        "正在协商密钥",
        "协商密钥成功",
        "正在认证",
        "用户在线"
    };
    VNET_USER_STATUS_E enStatus = g_enVnetcUserStatus;

    if (enStatus >= VNET_USER_STATUS_MAX)
    {
        return "";
    }

    return apcStatusString[enStatus];
}


CHAR * VNETC_User_GetReasonString()
{
    static CHAR * apcReasonString[VNET_USER_REASON_MAX] =
    {
        "",
            
        "版本号不兼容",
        "动态用户数达到上限",
        "服务器忙",
        "需要充值",
        "暂停服务",
        "用户已经登录",
        "服务器无响应",
        "认证失败",
        "资源不足",
        "连接失败"
    };
    static CHAR szReason[32];
    VNET_USER_REASON_E enReason = g_enVnetcUserReason;

    if (enReason >= VNET_USER_REASON_MAX)
    {
        snprintf(szReason, sizeof(szReason), "错误码:%d", enReason);
        return szReason;
    }

    return apcReasonString[enReason];
}


