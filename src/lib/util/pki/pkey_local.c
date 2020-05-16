/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2015-12-15
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/md5_utl.h"
#include "utl/pki_utl.h"
#include "utl/rsa_utl.h"
#include "utl/dsa_utl.h"
#include "openssl/ssl.h"
#include "openssl/pkcs12.h"

#include "pki_algo.h"
#include "pkey_local.h"
#include "pki_pubkey.h"


/*****************************************************************************
  Description: 生成RSA密钥对
        Input: IN UINT uiKeySize ,密钥长度,512-2048  
       Output: 无
       Return: EVP_PKEY *,密钥数据，若失败则返回NULL
      Caution: 
*****************************************************************************/
EVP_PKEY *PKEY_GenerateRSAKey(IN UINT uiKeySize)
{
    INT iRet;
    RSA *pstRSAKey;    
    EVP_PKEY *pstPKey;

    /* 生成RSA密钥，并使pstRSAKey指向新生成的密钥 */
    pstRSAKey = RSA_BuildKey(uiKeySize);
    
    if (NULL == pstRSAKey)
    {    
        return NULL;
    }
    /* 创建EVP_PKEY结构，并使指针pstPKey指向它 */
    pstPKey = EVP_PKEY_new();
    if (NULL == pstPKey)
    {
        /* 释放pstRSAKey*/
        RSA_free(pstRSAKey);
        return NULL;
    }
    /* 通过pstRSAKey填充pstPKey，返回填充长度到iRet，这里不能使用EVP_PKEY_set1_RSA赋值，否则内存泄露*/
    iRet = EVP_PKEY_assign_RSA(pstPKey, pstRSAKey);
    if (PKEY_EVP_SET_SUCCESS != iRet)
    {
        /* 释放pstRSAKey*/
        RSA_free(pstRSAKey);
            
        /* 释放pstPKey*/
        EVP_PKEY_free(pstPKey);            
        return NULL;
    }
    
    return pstPKey;
}

/*****************************************************************************
  Description: 生成DSA密钥对
        Input: IN UINT uiKeySize ,密钥长度,512-2048   
       Output: 无
       Return: EVP_PKEY *,密钥数据，若失败则返回NULL
*****************************************************************************/
EVP_PKEY *PKEY_GenerateDSAKey(IN UINT uiKeySize)
{
    INT iRet;
    DSA *pstDSAKey;
    EVP_PKEY *pstPKey;

    /* 得到生成DSA密钥所需要的参数,并使pstDSAKey指向它 */
    pstDSAKey = DSA_BuildKey((INT) uiKeySize);
    if (NULL == pstDSAKey)
    {    
        return NULL;
    }
    /*生成DSA密钥，返回值为密钥长度，赋给iRet */
    iRet = DSA_generate_key(pstDSAKey);   
    if (0 == iRet)
    {
        /* 释放pstDSAKey*/
        DSA_free(pstDSAKey);      
        return NULL;
    }
    /* 创建EVP_PKEY结构，并使指针pstPKey指向它 */
    pstPKey = EVP_PKEY_new();
    if (NULL == pstPKey)
    {
        /* 释放pstDSAKey*/
        DSA_free(pstDSAKey); 
        return NULL;
    }

    /* 通过pstDSAKey填充pstPKey，返回填充长度到iRet*/
    iRet = EVP_PKEY_assign_DSA(pstPKey, pstDSAKey);
    if (PKEY_EVP_SET_SUCCESS != iRet)
    {
        /* 释放pstDSAKey*/
        DSA_free(pstDSAKey);     
        /* 释放pstPKey*/
        EVP_PKEY_free(pstPKey);     
        return NULL;
    }
        
    return pstPKey;
}


/*****************************************************************************
  Description: 生成ECDSA密钥对
        Input: IN INT iNid    椭圆曲线NID     
       Output: 无
       Return: EVP_PKEY *,密钥数据，若失败则返回NULL
*****************************************************************************/
EVP_PKEY *PKEY_GenerateECKey(IN INT iNid)
{
    EVP_PKEY *pstPKey = NULL;

#ifdef OPENSSL_EC_NAMED_CURVE
    INT iRet;
    EC_KEY *pstECKEY;

    DBGASSERT((NID_X9_62_prime192v1 == iNid) || ( NID_X9_62_prime256v1 == iNid) || (NID_secp384r1 == iNid));

    /* 生成EC_KEY结构，并使pstECKEY指向它 */
    pstECKEY = EC_KEY_new_by_curve_name (iNid);
    if (NULL == pstECKEY)
    {
        return NULL;
    }
    EC_GROUP_set_asn1_flag ((EC_GROUP *) EC_KEY_get0_group (pstECKEY), OPENSSL_EC_NAMED_CURVE);
    /* 生成ECDSA密钥 */
    iRet = EC_KEY_generate_key(pstECKEY);
    if (0 == iRet)
    {
        EC_KEY_free(pstECKEY);  
        return NULL;
    }

    /* 创建EVP_PKEY结构，并使指针pstPKey指向它 */
    pstPKey = EVP_PKEY_new();
    if (NULL == pstPKey)
    {
        EC_KEY_free(pstECKEY);  
        return NULL;
    }
    
    /* 通过pstECKEY填充pstPKey，返回填充长度到iRet*/
    iRet = EVP_PKEY_assign_EC_KEY(pstPKey, pstECKEY);    
    if (PKEY_EVP_SET_SUCCESS != iRet)
    {
        /* 释放pstECKEY*/
        EC_KEY_free(pstECKEY);
        /* 释放pstPKey*/
        EVP_PKEY_free(pstPKey); 
        return NULL;
    }

#endif
    
    return pstPKey;
}

