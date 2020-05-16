
#ifndef __VNET_OFFLINE_H_
#define __VNET_OFFLINE_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef struct
{
    USHORT usVer;
    USHORT usType;
}VNET_OFFLINE_HEAD_S;

typedef struct
{
    VNET_OFFLINE_HEAD_S stHead;
}VNET_OFFLINE_S;

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNET_OFFLINE_H_*/


