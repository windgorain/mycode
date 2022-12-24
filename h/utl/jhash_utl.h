/******************************************************************************
* Copyright (C), 2000-2006,  Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2017-6-2
* Description: 
* History:     
******************************************************************************/

#ifndef __JHASH_UTL_H_
#define __JHASH_UTL_H_

#include "utl/num_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define JHASH_INITVAL		0xdeadbeef

#define JHASH_MIX(a, b, c)    { \
	a -= c;  a ^= NUM_Rol32(c, 4);  c += b;	\
	b -= a;  b ^= NUM_Rol32(a, 6);  a += c;	\
	c -= b;  c ^= NUM_Rol32(b, 8);  b += a;	\
	a -= c;  a ^= NUM_Rol32(c, 16); c += b;	\
	b -= a;  b ^= NUM_Rol32(a, 19); a += c;	\
	c -= b;  c ^= NUM_Rol32(b, 4);  b += a;	\
}

#define JHASH_FINAL(a, b, c)  { \
	c ^= b; c -= NUM_Rol32(b, 14);		\
	a ^= c; a -= NUM_Rol32(c, 11);		\
	b ^= a; b -= NUM_Rol32(a, 25);		\
	c ^= b; c -= NUM_Rol32(b, 16);		\
	a ^= c; a -= NUM_Rol32(c, 4);		\
	b ^= a; b -= NUM_Rol32(a, 14);		\
	c ^= b; c -= NUM_Rol32(b, 24);		\
}

UINT JHASH_GeneralBuffer(void *key, UINT length, UINT initval);
UINT JHASH_U32Buffer(UINT *k, UINT length, UINT initval);

static inline UINT JHASH_NWORDS(UINT a, UINT b, UINT c, UINT initval)
{
	a += initval;
	b += initval;
	c += initval;
	JHASH_FINAL(a, b, c);
	return c;
}

static inline UINT JHASH_3Words(UINT a, UINT b, UINT c, UINT initval)
{
	return JHASH_NWORDS(a, b, c, initval + JHASH_INITVAL + (3 << 2));
}

static inline UINT JHASH_2Words(UINT a, UINT b, UINT initval)
{
	return JHASH_NWORDS(a, b, 0, initval + JHASH_INITVAL + (2 << 2));
}

static inline UINT JHASH_Word(UINT a, UINT initval)
{
	return JHASH_NWORDS(a, 0, 0, initval + JHASH_INITVAL + (1 << 2));
}

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__JHASH_UTL_H_*/


