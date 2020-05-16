

#ifndef __VNET_USER_STATUS_H_
#define __VNET_USER_STATUS_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

/* 用户状态 */
typedef enum
{
    VNET_USER_STATUS_INIT = 0,                      /* 初始化状态 */
    VNET_USER_STATUS_OFFLINE,                       /* 用户不在线 */
    VNET_USER_STATUS_CONNECTING,                    /* 正在连接服务器 */
    VNET_USER_STATUS_GET_VER_ING,                   /* 正在获取版本号 */
    VNET_USER_STATUS_GET_VER_OK,                    /* 获取版本号成功 */
    VNET_USER_STATUS_CONSULT_SEC_ING,               /* 正在协商密钥 */
    VNET_USER_STATUS_CONSULT_SEC_OK,                /* 协商密钥成功 */
    VNET_USER_STATUS_AUTH_ING,                      /* 正在认证  */
    VNET_USER_STATUS_ONLINE,                        /* 用户在线 */    

    VNET_USER_STATUS_MAX
}VNET_USER_STATUS_E;

/* 失败原因 */
typedef enum
{
    VNET_USER_REASON_NONE = 0,                      /* 无错误.用于认证成功,或者是还未开始认证. */

    VNET_USER_REASON_VER_NOT_MATCH,                 /* 版本号不兼容 */
    VNET_USER_REASON_DYNAMIC_USER_LIMIT_REACHED,    /* 动态用户数达到上限 */
    VNET_USER_REASON_BUSY,                          /* 服务器忙 */
    VNET_USER_REASON_NEED_MONEY,                    /* 需要充值 */
    VNET_USER_REASON_TMP_STOP_SERVICE,              /* 暂停服务 */
    VNET_USER_REASON_ALREADY_LOGIN,                 /* 用户已经登录 */
    VNET_USER_REASON_SERVER_NO_ACK,                 /* 服务器无响应 */
    VNET_USER_REASON_AUTH_FAILED,                   /* 认证失败 */
    VNET_USER_REASON_NO_RESOURCE,                   /* 资源不足 */
    VNET_USER_REASON_CONNECT_FAILED,                /* 连接失败 */

    VNET_USER_REASON_MAX
}VNET_USER_REASON_E;

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNET_USER_STATUS_H_*/

