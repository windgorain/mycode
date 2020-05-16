/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-7-22
* Description: 
* History:     
******************************************************************************/
/* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_PASSWD

#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/cff_utl.h"
#include "utl/md5_utl.h"
#include "utl/data2hex_utl.h"
#include "utl/lvm_utl.h"
#include "utl/passwd_utl.h"


/* 无损加密 */
BS_STATUS PW_Base64Encrypt(IN CHAR *szClearText, OUT CHAR *szCipher, IN ULONG ulCipherSize)
{
    ULONG ulLen;
    DES_cblock stPwIov = {1,2,3,4,5,6,7,8};
    LVM_S stLvm;
    UCHAR *pucData;
    UINT uiCipherLen;

    ulLen = strlen(szClearText) + 1;  /* 把最后的结束符也加密, 便于解密后以'\0'结束长度正确 */

    BS_DBGASSERT(ulCipherSize > PW_BASE64_CIPHER_LEN(ulLen));

    uiCipherLen = DES_CIPHER_LEN(ulLen);
    
    pucData = LVM_Malloc(&stLvm, uiCipherLen);
    if (NULL == pucData)
    {
        return BS_NO_MEMORY;
    }

    DES_Ede3CbcEncrypt((UCHAR*)szClearText, pucData, ulLen,
        DES_GetSysKey1(), DES_GetSysKey2(), DES_GetSysKey3(), &stPwIov, TRUE);

    BASE64_Encode(pucData, uiCipherLen, szCipher);

    LVM_Free(&stLvm);

	return BS_OK;
}

/* 无损解密 */
BS_STATUS PW_Base64Decrypt(IN CHAR *szCipher, OUT CHAR *szClearText, IN ULONG ulClearSize)
{
    UINT uiLen;
    DES_cblock stPwIov = {1,2,3,4,5,6,7,8};
    LVM_S stLvmCipher;
    LVM_S stLvmClear;    
    UCHAR *pucCipher;
    UCHAR *pucClear;

    uiLen = strlen(szCipher);

    pucCipher = LVM_Malloc(&stLvmCipher, uiLen + 1);
    if (NULL == pucCipher)
    {
        return BS_NO_MEMORY;
    }

    pucClear = LVM_Malloc(&stLvmClear, uiLen + 1);
    if (NULL == pucClear)
    {
        LVM_Free(&stLvmCipher);
        return BS_NO_MEMORY;
    }

    uiLen = BASE64_Decode(szCipher, uiLen, pucCipher);
    if (BASE64_INVALD == uiLen)
    {
        LVM_Free(&stLvmCipher);
        LVM_Free(&stLvmClear);
        return BS_ERR;
    }

    DES_Ede3CbcEncrypt(pucCipher, pucClear, uiLen,
        DES_GetSysKey1(), DES_GetSysKey2(), DES_GetSysKey3(), &stPwIov, FALSE);

    TXT_Strlcpy(szClearText, (CHAR*)pucClear, ulClearSize);

    LVM_Free(&stLvmCipher);
    LVM_Free(&stLvmClear);

    return BS_OK;
}

/* 无损加密 */
BS_STATUS PW_HexEncrypt(IN CHAR *szClearText, OUT CHAR *szCipher, IN ULONG ulCipherSize)
{
    ULONG ulLen;
    DES_cblock stPwIov = {1,2,3,4,5,6,7,8};
    LVM_S stLvm;
    UCHAR *pucData;
    UINT uiCipherLen;

    ulLen = strlen(szClearText) + 1;

    BS_DBGASSERT(ulCipherSize > PW_HEX_CIPHER_LEN(ulLen));

    uiCipherLen = DES_CIPHER_LEN(ulLen);

    pucData = LVM_Malloc(&stLvm, uiCipherLen);
    if (NULL == pucData)
    {
        return BS_NO_MEMORY;
    }

    DES_Ede3CbcEncrypt((UCHAR*)szClearText, pucData, ulLen,
        DES_GetSysKey1(), DES_GetSysKey2(), DES_GetSysKey3(), &stPwIov, TRUE);

    DH_Data2HexString(pucData, uiCipherLen, szCipher);

    LVM_Free(&stLvm);

	return BS_OK;
}

/* 无损解密 */
BS_STATUS PW_HexDecrypt(IN CHAR *szCipher, OUT CHAR *szClearText, IN ULONG ulClearSize)
{
    UINT uiLen;
    DES_cblock stPwIov = {1,2,3,4,5,6,7,8};
    LVM_S stLvmCipher;
    LVM_S stLvmClear;    
    UCHAR *pucCipher;
    UCHAR *pucClear;

    uiLen = strlen(szCipher) / 2;

    pucCipher = LVM_Malloc(&stLvmCipher, uiLen + 1);
    if (NULL == pucCipher)
    {
        return BS_NO_MEMORY;
    }

    pucClear = LVM_Malloc(&stLvmClear, uiLen + 1);
    if (NULL == pucClear)
    {
        LVM_Free(&stLvmCipher);
        return BS_NO_MEMORY;
    }

    uiLen = DH_Hex2Data(szCipher, uiLen, pucCipher);
    if (BASE64_INVALD == uiLen)
    {
        LVM_Free(&stLvmCipher);
        LVM_Free(&stLvmClear);
        return BS_ERR;
    }

    DES_Ede3CbcEncrypt(pucCipher, pucClear, uiLen,
        DES_GetSysKey1(), DES_GetSysKey2(), DES_GetSysKey3(), &stPwIov, FALSE);

    TXT_Strlcpy(szClearText, (CHAR*)pucClear, ulClearSize);

    LVM_Free(&stLvmCipher);
    LVM_Free(&stLvmClear);

    return BS_OK;
}

/* 有损加密 */
BS_STATUS PW_Md5Encrypt(IN CHAR *szClearText, OUT CHAR szCipherText[PW_MD5_ENCRYPT_LEN + 1])
{
    UCHAR aucMd5[MD5_LEN];
    UCHAR aucCipher[MD5_LEN];
    DES_cblock stPwIov = {1,2,3,4,5,6,7,8};

    MD5_Create((UCHAR*)szClearText, strlen(szClearText), aucMd5);

    DES_Ede3CbcEncrypt(aucMd5, aucCipher, MD5_LEN,
        DES_GetSysKey1(), DES_GetSysKey2(), DES_GetSysKey3(), &stPwIov, TRUE);

    DH_Data2HexString(aucCipher, MD5_LEN, szCipherText);

    return BS_OK;
}

/* 有损加密的比较 */
BOOL_T PW_Md5Check(IN CHAR *szClearText, IN CHAR *pcCipherText)
{
    CHAR szNewCipherText[PW_MD5_ENCRYPT_LEN + 1];

    PW_Md5Encrypt(szClearText, szNewCipherText);

    if (strcmp(pcCipherText, szNewCipherText) == 0)
    {
        return TRUE;
    }

    return FALSE;
}

