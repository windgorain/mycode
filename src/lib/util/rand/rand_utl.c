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
    static int is_sranded = 0;

    if (! is_sranded) {
        is_sranded = 1;
        srand(time(NULL));
    }

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

