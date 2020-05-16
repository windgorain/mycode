
#ifndef __VNET_ICP_DEF_H_
#define __VNET_ICP_DEF_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef struct
{
    USHORT usVer;
    USHORT usReserved;
    UINT uiIcpLocalIp;      /* 客户端IP地址, 网络序 */
}VNET_ICP_REQUEST_HEAD_S;

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNET_ICP_DEF_H_*/


