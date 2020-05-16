/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-6-8
* Description: 
* History:     
******************************************************************************/

#ifndef __WS_TRANS_MEMPOOL_H_
#define __WS_TRANS_MEMPOOL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

BS_STATUS WS_TransMemPool_Init(IN WS_TRANS_S *pstTrans);
VOID WS_TransMemPool_Fini(IN WS_TRANS_S *pstTrans);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__WS_TRANS_MEMPOOL_H_*/


