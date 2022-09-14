/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-8-18
* Description: 
* History:     
******************************************************************************/

#ifndef __BIT_OPT_H_
#define __BIT_OPT_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#ifdef IN_WINDOWS
#define	ROTATE(a,n)	(_lrotr(a,n))   /* 将一个无符号长整形数右循环移位n位 */
#elif defined(__GNUC__) && __GNUC__>=2 
#if defined(__i386) || defined(__i386__) || defined(__x86_64) || defined(__x86_64__)
#define ROTATE(a,n)	({ register unsigned int ret;	\
				asm ("rorl %1,%0"	\
					: "=r"(ret)	\
					: "I"(n),"0"(a)	\
					: "cc");	\
			   ret;				\
			})
#endif
#endif

#ifndef ROTATE
#define	ROTATE(a,n)	(((a)>>(n))+((a)<<(32-(n))))
#endif

#define BIT_ISSET(ulFlag, ulBits)  (((ulFlag) & (ulBits)) != 0)
#define BIT_MATCH(ulFlag, ulBits)  (((ulFlag) & (ulBits)) == (ulBits))
#define BIT_SET(ulFlag, ulBits)  ((ulFlag) |= (ulBits))
#define BIT_CLR(ulFlag, ulBits)  ((ulFlag) &= ~(ulBits))
/* 翻转指定位 */
#define BIT_TURN(flag, bits) ((flag) ^= (bits))
/* 将指定位设置为指定值 */
#define BIT_SETTO(ulFlag, ulBitMask, ulToBits) ((ulFlag) = (((ulFlag) & (~(ulBitMask))) | ((ulToBits) & (ulBitMask))))

#define BIT_TEST BIT_ISSET
#define BIT_RESET BIT_CLR

/* 仅仅保留最低位, 如0x110, 变为0x010 */
#define BIT_ONLY_LAST(a) ((a) & (-(a)))

/* 获取最低位的index, 比如 0x1返回0, 0x2返回1, 0x4返回2 */
static inline int Bit_GetLastIndex(UINT num)
{
    int i;

    for (i=0; i<32; i++) {
        if (num & (1 << i)) { 
            return i;
        }
    }

    return -1;
}

/* 获取最高位的index */
static inline int Bit_GetFirstIndex(UINT num)
{
    int i;

    for (i=31; i>=0; i--) {
        if (num & (1 << i)) { 
            return i;
        }
    }

    return -1;
}

#ifdef __cplusplus
        }
#endif /* __cplusplus */

#endif /*__BIT_OPT_H_*/


