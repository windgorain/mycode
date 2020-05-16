
#ifndef __VNETC_IP_FWD_H_
#define __VNETC_IP_FWD_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

BS_STATUS VNETC_IpFwd_Input (IN MBUF_S *pstMbuf);
BS_STATUS VNETC_IpFwd_Output(IN MBUF_S *pstMbuf);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETC_IP_FWD_H_*/


