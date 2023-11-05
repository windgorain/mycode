/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-9-28
* Description: 
* History:     
******************************************************************************/

#ifndef __WS_EVENT_H_
#define __WS_EVENT_H_

#ifdef __cplusplus
    extern "C" {
#endif 

typedef WS_EV_RET_E (*PF_WS_EventProcess)(IN WS_TRANS_S *pstTrans, IN UINT uiEvent);

BS_STATUS WS_Event_IssuEvent(IN WS_TRANS_S *pstTrans, IN UINT uiEvent);

#ifdef __cplusplus
    }
#endif 

#endif 


