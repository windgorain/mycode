/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2009-4-3
* Description: 
* History:     
******************************************************************************/

#ifndef __RAND_UTL_H_
#define __RAND_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

UINT RAND_Get(void);

UINT RAND_GetNonZero(void);

/* 获取/dev/urandom随机数 */
unsigned long RAND_GetRandom(void);

/* 改变熵值 */
VOID RAND_Entropy(IN UINT uiEntropy);

/* 生成一段随机内存 */
void RAND_Mem(OUT UCHAR *buf, int len);

static inline UINT RAND_FastGet(UINT *seed)
{
    *seed = *seed * 214013 + 2531011;
    return *seed;
}

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__RAND_UTL_H_*/


