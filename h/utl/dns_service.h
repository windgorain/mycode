
#ifndef __DNS_SERVICE_H_
#define __DNS_SERVICE_H_

#include "utl/dns_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef HANDLE DNS_SERVICE_HANDLE;

/* struct */
typedef struct
{
    UINT uiIP;  /* IP地址 */
    CHAR szAuthorName[DNS_MAX_DOMAIN_NAME_LEN + 1];
}DNS_SERVICE_INFO_S;


DNS_SERVICE_HANDLE DNS_Service_Create(IN BOOL_T bCreateMutex);
VOID DNS_Service_Destory(IN DNS_SERVICE_HANDLE hDnsService);
BS_STATUS DNS_Service_AddName
(
    IN DNS_SERVICE_HANDLE hService,
    IN CHAR *pcDomainName,
    IN UINT uiIP/* 网络序 */
);
BS_STATUS DNS_Service_DelName(IN DNS_SERVICE_HANDLE hService, IN CHAR *pcDomainName);

BS_STATUS DNS_Service_GetInfoByName
(
    IN DNS_SERVICE_HANDLE hService,
    IN CHAR *pcDomainName,
    OUT DNS_SERVICE_INFO_S *pstInfo
);

/*
    OK: 成功
    ERR: 失败
    NO_SUCH: 未找到对应的域名
*/
BS_STATUS DNS_Service_PktInput
(
    IN DNS_SERVICE_HANDLE hDnsService,
    IN UCHAR *pucRequest,
    IN UINT uiRequestLen,
    OUT DNS_PKT_S *pstReply,    /* 返回的应答报文 */
    OUT UINT *puiReplyLen       /* 应答报文长度 */
);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /* __DNS_SERVICE_H_ */

