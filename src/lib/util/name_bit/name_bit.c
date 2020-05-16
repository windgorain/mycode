/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-11-28
* Description: 一个名字对应的bit位. 比如Debug Flag
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/name_bit.h"

UINT NameBit_GetBitByName(IN NAME_BIT_S *pstNameBits, IN CHAR *pcName)
{
    NAME_BIT_S *pstTmp;

    pstTmp = pstNameBits;

    while (pstTmp->pcName != NULL)
    {
        if (strcmp(pstTmp->pcName, pcName) == 0)
        {
            return pstTmp->uiBit;
        }

        pstTmp ++;
    }

    return 0;
}

