

#ifndef __VNETC_SES_H_
#define __VNETC_SES_H_

#include "utl/ses_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif 

typedef enum
{
    VNETC_SES_PROPERTY_NODE_ID = 0,

    VNETC_SES_PROPERTY_MAX
}VNETC_SES_PROPERTY_E;

BS_STATUS VNETC_SES_Init();
UINT VNETC_SES_CreateClient(IN VNETC_PHY_CONTEXT_S *pstPhyContext);
SES_TYPE_E VNETC_SES_GetType(IN UINT uiSesID);
BS_STATUS VNETC_SES_SetEventNotify(IN UINT uiSesID, IN PF_SES_EVENT_NOTIFY pfEventNotify, IN USER_HANDLE_S *pstUserHandle);
BS_STATUS VNETC_SES_RegCloseNotify(IN PF_SES_CLOSE_NOTIFY_FUNC pfFunc,IN USER_HANDLE_S * pstUserHandle);
BS_STATUS VNETC_SES_SetProperty(IN UINT uiSesID, IN UINT uiPropertyIndex, IN HANDLE hValue);
HANDLE VNETC_SES_GetProperty(IN UINT uiSesID, IN UINT uiPropertyIndex);
BS_STATUS VNETC_SES_Connect(IN UINT uiSesID);
BS_STATUS VNETC_SES_PktInput(IN UINT ulIfIndex, IN MBUF_S *pstMbuf);
BS_STATUS VNETC_SES_SendPkt(IN UINT uiSesID, IN MBUF_S *pstMbuf);
VOID VNETC_SES_Close(IN UINT uiSesID);
BS_STATUS VNETC_SES_SetOpt(IN UINT uiSesID, IN UINT uiOpt, IN VOID *pValue);
VNETC_PHY_CONTEXT_S * VNETC_SES_GetPhyContext(IN UINT uiSesID);
UINT VNETC_SES_GetIfIndex(IN UINT uiSesID);

#ifdef __cplusplus
    }
#endif 

#endif 


