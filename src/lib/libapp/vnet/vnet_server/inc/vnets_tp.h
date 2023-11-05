
#ifndef __VNETS_TP_H_
#define __VNETS_TP_H_

#include "utl/tp_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif 

typedef enum
{
    VNETS_TP_PROPERTY_NODE = 0,

    VNETS_TP_PROPERTY_MAX
}VNETS_TP_PROPERTY_E;

BS_STATUS VNETS_TP_Init();

BS_STATUS VNETS_TP_Init2();

BS_STATUS VNETS_TP_RegCloseEvent(IN PF_TP_CLOSE_NOTIFY_FUNC pfFunc, IN USER_HANDLE_S * pstUserHandle);

BS_STATUS VNETS_TP_SetProperty(IN UINT uiTPID, IN UINT uiIndex, IN HANDLE hValue);

HANDLE VNETS_TP_GetProperty(IN UINT uiTPID, IN UINT uiIndex);

BS_STATUS VNETS_TP_Input(IN MBUF_S *pstMbuf);

BS_STATUS VNETS_TP_Output(IN UINT uiTpId, IN MBUF_S *pstMbuf);

#ifdef __cplusplus
    }
#endif 

#endif 


