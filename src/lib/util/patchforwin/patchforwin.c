/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-5-13
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

long _ftol( double );
long _ftol2( double dblSource )
{
    return _ftol(dblSource);
}


long _aulldiv(unsigned long, unsigned long);
long _aulldvrm(unsigned long divisor, unsigned long dividend)
{
    return _aulldiv(divisor, dividend);
}

