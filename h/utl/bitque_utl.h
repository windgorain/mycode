/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-3-27
* Description: 
* History:     
******************************************************************************/

#ifndef __BITQUE_UTL_H_
#define __BITQUE_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef struct
{
    BITMAP_S stBitMap;
    int size;    /* 最大容量 */
    int count;   /* 队列中有多少个位 */
    int read_index;
    int write_index;
}BITQUE_S;

BS_STATUS BITQUE_Init(BITQUE_S *pstBitQue, int size/* 支持多少个bit */);
VOID BITQUE_Fini(IN BITQUE_S *pstBitQue);
/* 返回占用的位置 */
int BITQUE_Enque(BITQUE_S *pstBitQue, int bit_value);
/* 返回-1表示没有弹出 */
int BITQUE_Deque(BITQUE_S *pstBitQue);
int BITQUE_Get(BITQUE_S *pstBitQue, int index);
/* 返回应该返回的值,但是不真正弹出 */
int BITQUE_TestDeque(BITQUE_S *pstBitQue);


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__BITQUE_UTL_H_*/


