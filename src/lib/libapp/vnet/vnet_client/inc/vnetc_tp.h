
#ifndef __VNETC_TP_H_
#define __VNETC_TP_H_

#include "utl/mbuf_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

BS_STATUS VNETC_TP_Init();

BS_STATUS VNETC_TP_Init2();

BS_STATUS VNETC_TP_ConnectServer();

VOID VNETC_TP_TriggerKeepAlive();

BS_STATUS VNETC_TP_PktInput(IN MBUF_S *pstMbuf);

BS_STATUS VNETC_TP_PktOutput(IN UINT uiTpId, IN MBUF_S *pstMbuf);

BS_STATUS VNETC_TP_Stop();

UINT VNETC_TP_GetC2STP();

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETC_TP_H_*/


