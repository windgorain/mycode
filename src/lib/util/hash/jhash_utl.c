/******************************************************************************
* Copyright (C), 2000-2006,  Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2017-6-2
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/jhash_utl.h"

UINT JHASH_GeneralBuffer(void *key, UINT length, UINT initval)
{
	UINT a, b, c;
	UCHAR *k = key;
    UINT *d;

	a = b = c = JHASH_INITVAL + length + initval;

	while (length > 12) {
        d = (void*)k;
		a += d[0];
		b += d[1];
		c += d[2];
		JHASH_MIX(a, b, c);
		length -= 12;
		k += 12;
	}

	switch (length) {
	case 12: c += (UINT)k[11]<<24;
	case 11: c += (UINT)k[10]<<16;
	case 10: c += (UINT)k[9]<<8;
	case 9:  c += k[8];
	case 8:  b += (UINT)k[7]<<24;
	case 7:  b += (UINT)k[6]<<16;
	case 6:  b += (UINT)k[5]<<8;
	case 5:  b += k[4];
	case 4:  a += (UINT)k[3]<<24;
	case 3:  a += (UINT)k[2]<<16;
	case 2:  a += (UINT)k[1]<<8;
	case 1:  a += k[0];
		 JHASH_FINAL(a, b, c);
	case 0:
		break;
	}

	return c;
}


UINT JHASH_U32Buffer(UINT *k, UINT length, UINT initval)
{
	UINT a, b, c;

	a = b = c = JHASH_INITVAL + (length<<2) + initval;

	while (length > 3) {
		a += k[0];
		b += k[1];
		c += k[2];
		JHASH_MIX(a, b, c);
		length -= 3;
		k += 3;
	}

	switch (length) {
	case 3: c += k[2];
	case 2: b += k[1];
	case 1: a += k[0];
		JHASH_FINAL(a, b, c);
	case 0:	
		break;
	}

	return c;
}


