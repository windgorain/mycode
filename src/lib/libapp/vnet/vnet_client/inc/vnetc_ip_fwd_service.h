

#ifndef __VNETC_IP_FWD_SERVICE_H_
#define __VNETC_IP_FWD_SERVICE_H_

#include "utl/mbuf_utl.h"
#include "utl/ip_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif 

typedef enum
{
    VNETC_IPFWD_SERVICE_PRE_ROUTING,               
    VNETC_IPFWD_SERVICE_LOCAL_IN,                  
    VNETC_IPFWD_SERVICE_FORWARD,                   
    VNETC_IPFWD_SERVICE_LOCAL_OUT,                 
    VNETC_IPFWD_SERVICE_POST_ROUTING_BEFOREFRAG,   
    VNETC_IPFWD_SERVICE_POST_ROUTING_AFTERFRAG,    
    VNETC_IPFWD_SERVICE_PRE_RELAY,                 
    VNETC_IPFWD_SERVICE_RELAY_RECEIVE,             

    VNETC_IPFWD_SERVICE_PHASE_MAX
}VNETC_IPFWD_SERVICE_PHASE_E;

typedef enum
{
    VNETC_IPFWD_SERVICE_RET_CONTINUE = 0,   
    VNETC_IPFWD_SERVICE_RET_TAKE_OVER,      
    VNETC_IPFWD_SERVICE_RET_ERR,            

    VNETC_IPFWD_SERVICE_RET_MAX
}VNETC_IPFWD_SERVICE_RET_E;

typedef VNETC_IPFWD_SERVICE_RET_E (*PF_VNETC_IPFWD_SERVICE_FUNC)(IN IP_HEAD_S *pstIpHead, IN MBUF_S *pstMbuf);


BS_STATUS VNETC_IpFwdService_Reg
(
    IN VNETC_IPFWD_SERVICE_PHASE_E ePhase,
    IN PF_VNETC_IPFWD_SERVICE_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
);

VNETC_IPFWD_SERVICE_RET_E VNETC_IpFwdService_Handle
(
    IN VNETC_IPFWD_SERVICE_PHASE_E ePhase,
    IN IP_HEAD_S *pstIpHead,
    IN MBUF_S *pstMbuf
);

BS_STATUS VNETC_IpFwdService_Init();

#ifdef __cplusplus
    }
#endif 

#endif 




