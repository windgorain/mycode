/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-11-20
* Description: 
* History:     
******************************************************************************/

#ifndef __SEM_UTL_H_
#define __SEM_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef HANDLE SEM_HANDLE;

/* 主要用于递归取互斥信号量. 如果不存在递归情况,建议使用计数器信号量, 效率比这个高一倍左右 */
SEM_HANDLE SEM_MCreate (const char *pcName);
/* 当lInitNum为1时, 就相当于不支持递归SEM_P的互斥信号量. 如果使用者不存在递归情况,建议使用它 */
SEM_HANDLE SEM_CCreate (const char *pcName, IN INT iInitNum);
VOID SEM_Destory(IN SEM_HANDLE hSem);
BS_STATUS SEM_P (IN SEM_HANDLE hSem, IN BS_WAIT_E eWaitMode, IN UINT ulMilliseconds);
/* 在取一个信号量的同时，释放另外一个信号量 */
BS_STATUS SEM_PV
(
    IN SEM_HANDLE hSemToP,
    IN SEM_HANDLE hSemToV,
    IN BS_WAIT_E eWaitMode,
    IN UINT ulMilliseconds
);
BS_STATUS SEM_V(IN SEM_HANDLE hSem);
BS_STATUS SEM_VAll(IN SEM_HANDLE hSem);
/* 有多少阻塞在上面 */
INT SEM_CountPending(IN SEM_HANDLE hSem);

#define SEM_LOCK_IF_EXIST(hSem)    do{if ((hSem) != NULL){SEM_P(hSem, BS_WAIT, BS_WAIT_FOREVER);}}while(0)
#define SEM_UNLOCK_IF_EXIST(hSem)  do{if ((hSem) != NULL){SEM_V(hSem);}}while(0)


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__SEM_UTL_H_*/


