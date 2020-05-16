#ifndef __VNETS_UDP_PHY_H_
#define __VNETS_UDP_PHY_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

BS_STATUS VNETS_UDP_PHY_Init
(
    IN UINT ulServerIp/* 主机序 */,
    IN USHORT usServerPort/* 主机序 */
);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETS_UDP_PHY_H_*/

