/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2015-12-15
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/md5_utl.h"
#include "utl/pki_utl.h"
#include "openssl/ssl.h"
#include "openssl/pkcs12.h"

#include "pki_algo.h"
#include "pkey_local.h"
#include "pki_pubkey.h"

typedef const EVP_MD *(* PKI_ALGO_DIGEST_METHOD_PF)(VOID);

STATIC BOOL_T g_iPkiAddAlgoOnce = FALSE;

/*****************************************************************************
    Func Name: PKI_ResvEvpMd
 Date Created: 2010/12/14 
       Author: l07557
  Description: 生成缺省的摘要算法
        Input: 获取缺省的摘要算法结构
       Return: NULL
      Caution: 
------------------------------------------------------------------------------
  Modification History                                                      
  DATE        NAME             DESCRIPTION                                  
  --------------------------------------------------------------------------
                                                                            
*****************************************************************************/
STATIC const EVP_MD *PKI_ResvEvpMd(VOID)
{
    return NULL;
}

STATIC PKI_ALGO_DIGEST_METHOD_PF g_apfPKIDigestMethod[PKI_HASH_ALGMETHOD_MAX] =
{
    PKI_ResvEvpMd,
    EVP_md5,        /* PKI_HASH_ALGMETHOD_MD5 */
    EVP_sha1,       /* PKI_HASH_ALGMETHOD_SHA1 */
    EVP_sha224,     /* PKI_HASH_ALGMETHOD_SHA224 */
    EVP_sha256,     /* PKI_HASH_ALGMETHOD_SHA256 */
    EVP_sha384,     /* PKI_HASH_ALGMETHOD_SHA384 */
    EVP_sha512,     /* PKI_HASH_ALGMETHOD_SHA512 */
    EVP_ripemd160   /* PKI_HASH_ALGMETHOD_RIPEMD */
};

/*****************************************************************************
  Description: 加载所有算法
*****************************************************************************/
STATIC VOID algo_add_all_algorithms(VOID)
{
    OpenSSL_add_all_algorithms();
    return;
}

/*****************************************************************************
    Func Name: PKI_ALGO_GetDigestMethodByHashAlgoId
 Date Created: 2011/5/28 
       Author: sunludong03130
  Description: 根据HASH算法的ID获取摘要方法
        Input: IN PKI_HASH_ALGMETHOD_E enHash  
       Output: 
       Return: const EVP_MD *    摘要方法
                     NULL        获取摘要方法失败
      Caution: 
------------------------------------------------------------------------------
  Modification History                                                      
  DATE        NAME             DESCRIPTION                                  
  --------------------------------------------------------------------------
                                                                            
*****************************************************************************/
const EVP_MD *PKI_ALGO_GetDigestMethodByHashAlgoId(IN PKI_HASH_ALGMETHOD_E enHash)
{
    DBGASSERT (enHash < PKI_HASH_ALGMETHOD_MAX);
    return g_apfPKIDigestMethod[enHash]();
}

/*****************************************************************************
  Description: 加载PKI需要的所有算法
      Caution: 需要加载算法的位置都可以调用此函数，此函数内部实现保证了对同一个
               进程，真正的加载处理只执行一次
*****************************************************************************/
VOID PKI_ALGO_add_all_algorithms(VOID)
{
    BOOL_T bNeedInit = FALSE;

    if (g_iPkiAddAlgoOnce == TRUE)
    {
        return;
    }

    if (g_iPkiAddAlgoOnce == FALSE)
    {
        g_iPkiAddAlgoOnce = TRUE;
        bNeedInit = TRUE;
    }

    if (bNeedInit == TRUE)
    {
        algo_add_all_algorithms();
    }
    return;
}


