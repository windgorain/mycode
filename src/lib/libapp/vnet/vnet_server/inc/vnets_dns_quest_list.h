
#ifndef __VNETS_DNS_LIST_H_
#define __VNETS_DNS_LIST_H_

#include "../inc/vnets_ses.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef HANDLE VNETS_DQL_HANDLE;

VNETS_DQL_HANDLE VNETS_DQL_Create();
VOID VNETS_DQL_Destory(IN VNETS_DQL_HANDLE *pstHandle);
BS_STATUS VNETS_DQL_AddMbuf(IN VNETS_DQL_HANDLE *pstHandle, IN MBUF_S *pstMbuf);
MBUF_S * VNETS_DQL_GetMbuf(IN VNETS_DQL_HANDLE *pstHandle);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETS_DNS_LIST_H_*/


