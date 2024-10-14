/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-8-18
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
#include "utl/aes_utl.h"


void * AES_GetSysKey(void)
{
    
    static UINT key[32] = {
        0xa7969198, 0x9e9198d1, 0xb396d1bc, 0x908f868d,
        0x9698978b, 0x12345678, 0x9abcdef0, 0xf1e2c3d4
    };

    return key;
}

void * AES_GetSysIv(void)
{
    
    static UINT sys_iv[AES_IV_SIZE] = { 0x4c695869, 0x6e67616e, 0x67c2a943, 0x6f707972 };
    return sys_iv;
}
