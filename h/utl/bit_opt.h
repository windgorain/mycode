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
#define	BIT_ROTATE(a,n)	(_lrotr(a,n))   /* 将一个无符号长整形数右循环移位n位 */
#elif defined(__GNUC__) && __GNUC__>=2 
#if defined(__i386) || defined(__i386__) || defined(__x86_64) || defined(__x86_64__)
#define BIT_ROTATE(a,n)	({ register unsigned int ret;	\
				asm ("rorl %1,%0"	\
					: "=r"(ret)	\
					: "I"(n),"0"(a)	\
					: "cc");	\
			   ret;				\
			})
#endif
#endif

#ifndef BIT_ROTATE
#define	BIT_ROTATE(a,n)	(((a)>>(n))+((a)<<(32-(n))))
#endif

/* 检测是否设置了ulBits中的任意置1中的一位 */
#define BIT_ISSET(ulFlag, ulBits)  (((ulFlag) & (ulBits)) != 0)
/* 检测是否设置了ulBits中的所有置1的位 */
#define BIT_MATCH(ulFlag, ulBits)  (((ulFlag) & (ulBits)) == (ulBits))
/* 设置ulBits中置1的位为1 */
#define BIT_SET(ulFlag, ulBits)  ((ulFlag) |= (ulBits))
/* 清除ulBits中置1的位 */
#define BIT_CLR(ulFlag, ulBits)  ((ulFlag) &= ~(ulBits))

#define BIT_TEST(flag, bits) BIT_ISSET(flag, bits)
#define BIT_RESET(flag, bits) BIT_CLR(flag, bits)

/* 将[begin, end]位设置为1 */
#define BIT_BUILD_RANGE(begin, end)  ((0xffffffff >> (31 - ((end) - (begin)))) << (begin))

/* 将off, size位设置为1 */
#define BIT_BUILD_OFF(off, size)  BIT_BUILD_RANGE(off, (size)-1)

/* 翻转指定位 */
#define BIT_TURN(flag, bits) ((flag) ^= (bits))

/* 将指定mask位设置为指定值 */
#define BIT_SETTO(ulFlag, ulBitMask, ulToBits) ((ulFlag) = (((ulFlag) & (~(ulBitMask))) | ((ulToBits) & (ulBitMask))))

/* 将指定mask<<offset位设置为指定值 */
#define BIT_OFF_SETTO(ulFlag, mask, v, off) BIT_SETTO(ulFlag, (mask)<<(off), (v)<<(off))

/* 将 [begin, end] 位设置为 v */
#define BIT_RANGE_SETTO(ulFlag, begin, end, v) BIT_SETTO(ulFlag, BIT_BUILD_RANGE(begin, end), (v)<<(begin))

/* 获取off, size位的值 */
#define BIT_GET_OFF(v, off, size) (((v) >> off) & BIT_BUILD_OFF(0, size))

/* 仅仅保留最低的被设置为1的位, 如0x110, 变为0x010 */
#define BIT_ONLY_LAST(a) ((a) & (-(a)))

/* 获取一个二进制数字中'1'的个数 */
static inline UINT BIT_Count1(UINT u)
{
    u = (u & 0x55555555) + ((u >> 1) & 0x55555555);
    u = (u & 0x33333333) + ((u >> 2) & 0x33333333);
    u = (u & 0x0F0F0F0F) + ((u >> 4) & 0x0F0F0F0F);
    u = (u & 0x00FF00FF) + ((u >> 8) & 0x00FF00FF);
    u = (u & 0x0000FFFF) + ((u >> 16) & 0x0000FFFF);
    return u;
}

/* 描述bit字段 */
typedef struct {
    U32 off;
    U32 size;
}BIT_DESC_S;

/* 获取最低位的index, 比如 0x1返回0, 0x2返回1, 0x4返回2 */
static inline int BIT_GetLowIndex(UINT num)
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
static inline int BIT_GetHighIndex(UINT num)
{
    int i;

    for (i=31; i>=0; i--) {
        if (num & (1 << i)) { 
            return i;
        }
    }

    return -1;
}

/* 获取最低位的index, 比如 0x1返回0, 0x2返回1, 0x4返回2 */
static inline int BIT_GetLowIndex64(U64 num)
{
    int i;

    for (i=0; i<64; i++) {
        if (num & (1 << i)) { 
            return i;
        }
    }

    return -1;
}

/* 获取最高位的index */
static inline int BIT_GetHighIndex64(U64 num)
{
    int i;

    for (i=63; i>=0; i--) {
        if (num & (1 << i)) { 
            return i;
        }
    }

    return -1;
}

int BIT_GetHighIndexFrom(UINT num, UINT from /* 从哪位开始往下看 */);
char * BIT_SPrint(U32 v, U32 off, U32 size, OUT char *buf);
void BIT_Print(U32 v, U32 off, U32 size, PF_PRINT_FUNC func);
int BIT_XSPrint(U32 v, BIT_DESC_S *desc, int desc_num, OUT char *buf, U32 buf_size);
U8 BIT_ChangeOrder(U8 v);
UINT BIT_GetCount1(UINT u);

#ifdef __cplusplus
        }
#endif /* __cplusplus */

#endif /*__BIT_OPT_H_*/


