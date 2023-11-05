/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-7-22
* Description: 
* History:     
******************************************************************************/

#define RETCODE_FILE_NUM RETCODE_FILE_NUM_PASSWD

#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/cff_utl.h"
#include "utl/md5_utl.h"
#include "utl/data2hex_utl.h"
#include "utl/lvm_utl.h"
#include "utl/passwd_utl.h"


BS_STATUS PW_Base64Encrypt(IN CHAR *szClearText, OUT CHAR *szCipher, IN ULONG ulCipherSize)
{
    ULONG ulLen;
    LVM_S stLvm;
    UCHAR *pucData;
    UINT uiCipherLen;

    ulLen = strlen(szClearText) + 1;  

    if (ulCipherSize <= PW_BASE64_CIPHER_LEN(ulLen)) {
        BS_DBGASSERT(0);
        RETURN(BS_BAD_PARA);
    }

    uiCipherLen = AES_CIPHER_LEN(ulLen);
    
    pucData = LVM_Malloc(&stLvm, uiCipherLen);
    if (NULL == pucData) {
        return BS_NO_MEMORY;
    }

    int ret = AES_Cipher256(AES_GetSysKey(), AES_GetSysIv(), (void*)szClearText, ulLen, pucData, uiCipherLen, 1);
    if (ret < 0) {
        LVM_Free(&stLvm);
        return ret;
    }

    BASE64_Encode(pucData, ret, szCipher);

    LVM_Free(&stLvm);

	return BS_OK;
}


BS_STATUS PW_Base64Decrypt(IN CHAR *szCipher, OUT CHAR *szClearText, IN ULONG ulClearSize)
{
    UINT uiLen;
    int cipher_len;
    LVM_S stLvmCipher;
    UCHAR *pucCipher;

    uiLen = strlen(szCipher);

    pucCipher = LVM_Malloc(&stLvmCipher, uiLen + 1);
    if (NULL == pucCipher) {
        RETURN(BS_NO_MEMORY);
    }

    cipher_len = BASE64_Decode(szCipher, uiLen, pucCipher);
    if (cipher_len < 0) {
        LVM_Free(&stLvmCipher);
        RETURN(BS_ERR);
    }

    int ret = AES_Cipher256(AES_GetSysKey(), AES_GetSysIv(), (void*)pucCipher, cipher_len, (void*)szClearText, ulClearSize, 0);
    if (ret < 0) {
        LVM_Free(&stLvmCipher);
        return ret;
    }

    LVM_Free(&stLvmCipher);

    return BS_OK;
}


BS_STATUS PW_HexEncrypt(IN CHAR *szClearText, OUT CHAR *szCipher, IN ULONG ulCipherSize)
{
    ULONG ulLen;
    LVM_S stLvm;
    UCHAR *pucData;
    UINT uiCipherLen;

    ulLen = strlen(szClearText) + 1;

    if (ulCipherSize <= PW_HEX_CIPHER_LEN(ulLen)) {
        BS_DBGASSERT(0);
        RETURN(BS_BAD_PARA);
    }

    uiCipherLen = AES_CIPHER_LEN(ulLen);

    pucData = LVM_Malloc(&stLvm, uiCipherLen);
    if (NULL == pucData) {
        return BS_NO_MEMORY;
    }

    int ret = AES_Cipher256(AES_GetSysKey(), AES_GetSysIv(), (void*)szClearText, ulLen, pucData, uiCipherLen, 1);
    if (ret < 0) {
        LVM_Free(&stLvm);
        return ret;
    }

    DH_Data2HexString(pucData, ret, szCipher);

    LVM_Free(&stLvm);

	return BS_OK;
}


BS_STATUS PW_HexDecrypt(IN CHAR *szCipher, OUT CHAR *szClearText, IN ULONG ulClearSize)
{
    UINT uiLen;
    LVM_S stLvmCipher;
    UCHAR *pucCipher;

    uiLen = strlen(szCipher) / 2;

    pucCipher = LVM_Malloc(&stLvmCipher, uiLen + 1);
    if (NULL == pucCipher) {
        return BS_NO_MEMORY;
    }

    DH_Hex2Data(szCipher, uiLen*2, pucCipher);

    int ret = AES_Cipher256(AES_GetSysKey(), AES_GetSysIv(), (void*)pucCipher, uiLen, (void*)szClearText, ulClearSize, 0);
    if (ret < 0) {
        LVM_Free(&stLvmCipher);
        return ret;
    }

    LVM_Free(&stLvmCipher);

    return BS_OK;
}


BS_STATUS PW_Md5Encrypt(IN CHAR *szClearText, OUT CHAR szCipherText[PW_MD5_ENCRYPT_LEN + 1])
{
    UCHAR aucMd5[MD5_LEN];
    UCHAR aucCipher[MD5_LEN];

    MD5_Create((UCHAR*)szClearText, strlen(szClearText), aucMd5);

    int ret = AES_Cipher256(AES_GetSysKey(), AES_GetSysIv(), (void*)aucMd5, MD5_LEN, aucCipher, sizeof(aucCipher), 1);
    if (ret < 0) {
        return ret;
    }

    DH_Data2HexString(aucCipher, ret, szCipherText);

    return BS_OK;
}


BOOL_T PW_Md5Check(IN CHAR *szClearText, IN CHAR *pcCipherText)
{
    CHAR szNewCipherText[PW_MD5_ENCRYPT_LEN + 1];

    PW_Md5Encrypt(szClearText, szNewCipherText);

    if (strcmp(pcCipherText, szNewCipherText) == 0) {
        return TRUE;
    }

    return FALSE;
}

