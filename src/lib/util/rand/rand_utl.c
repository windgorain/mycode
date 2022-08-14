/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-8-1
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/time_utl.h"

UINT RAND_Get()
{
    return rand();
}

UINT RAND_GetNonZero()
{
    UINT uiRand;

    do {
        uiRand = RAND_Get();
    }while (uiRand == 0);

    return uiRand;
}

VOID RAND_Entropy(IN UINT uiEntropy)
{
    UINT seed = RAND_Get() + uiEntropy;
    srand(seed);
}

#ifdef IN_UNIXLIKE
/* 获取/dev/urandom随机数 */
unsigned long RAND_GetRandom()
{
	int fd=0;
	unsigned long ul_num = 0;

	fd=open("/dev/urandom",O_RDONLY);
	if(fd<0) {
        return RAND_Get();
	}

	int ret=read(fd,&ul_num,sizeof(ul_num));
	if(ret<0){
		close(fd);
        return RAND_Get();
	}

	close(fd);

	return ul_num;
}
#else
unsigned long RAND_GetRandom()
{
    return RAND_Get();
}
#endif

static void rand_init()
{
    srand(time(NULL));
}

CONSTRUCTOR(init) {
    rand_init();
}

