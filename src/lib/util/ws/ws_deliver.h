/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-7-27
* Description: 
* History:     
******************************************************************************/

#ifndef __WS_DELIVER_H_
#define __WS_DELIVER_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef struct
{
    DLL_HEAD_S stDeliverList;       /* _WS_DELIVER_NODE_S */
}WS_DELIVER_TBL_S;

typedef struct
{
    DLL_NODE_S stLinkNode;
    UINT uiPriority;
    WS_DELIVER_TYPE_E enType;
    VOID *pKey;
    UINT uiKeyLen;
    UINT uiFlag;
    PF_WS_Deliver_Func pfFunc;
}_WS_DELIVER_NODE_S;

_WS_DELIVER_NODE_S * _WS_Deliver_Match(IN WS_TRANS_S *pstTrans);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__WS_DELIVER_H_*/


