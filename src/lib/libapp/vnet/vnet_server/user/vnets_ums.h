#ifndef __VNETS_UMS_H_
#define __VNETS_UMS_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

BS_STATUS _VNETS_UMS_Init();
BS_STATUS _VNETS_UMS_Add
(
    IN CHAR *pcUserName,
    IN MAC_ADDR_S *pstMac,
    IN UINT uiSesId
);
VOID _VNETS_UMS_Del
(
    IN CHAR *pcUserName,
    IN MAC_ADDR_S *pstMac
);
UINT _VNETS_UMS_GetSesId
(
    IN CHAR *pcUserName,
    IN MAC_ADDR_S *pstMac
);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETS_UMS_H_*/


