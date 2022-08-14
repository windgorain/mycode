#include "bs.h"

#include "utl/des_utl.h"
#include "openssl/des.h"

#include "des_locl.h"

VOID DES_SetKey(IN const_DES_cblock *key, OUT DES_key_schedule *schedule)
{
	DES_set_key_unchecked(key, schedule);
}

/* 获取系统内置的Key */
DES_key_schedule * DES_GetSysKey1()
{
    static DES_key_schedule stDesSysKey;
    static BOOL_T bDesSysKeyIsInit = FALSE;
    static const_DES_cblock stKeyData = {0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8};

    if (bDesSysKeyIsInit == FALSE)
    {
        DES_SetKey(&stKeyData, &stDesSysKey);
        bDesSysKeyIsInit = TRUE;
    }

    return &stDesSysKey;
}

DES_key_schedule * DES_GetSysKey2()
{
    static DES_key_schedule stDesSysKey;
    static BOOL_T bDesSysKeyIsInit = FALSE;
    static const_DES_cblock  stKeyData = {0x9,0xa,0xb,0xc,0xd,0xe,0xf,0x10};

    if (bDesSysKeyIsInit == FALSE)
    {
        DES_SetKey(&stKeyData, &stDesSysKey);
        bDesSysKeyIsInit = TRUE;
    }

    return &stDesSysKey;
}

DES_key_schedule * DES_GetSysKey3()
{
    static DES_key_schedule stDesSysKey;
    static BOOL_T bDesSysKeyIsInit = FALSE;
    static const_DES_cblock stKeyData = {0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18};

    if (bDesSysKeyIsInit == FALSE)
    {
        DES_SetKey(&stKeyData, &stDesSysKey);
        bDesSysKeyIsInit = TRUE;
    }

    return &stDesSysKey;
}

