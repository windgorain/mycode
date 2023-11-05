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


STATIC const EVP_MD *PKI_ResvEvpMd(VOID)
{
    return NULL;
}

STATIC PKI_ALGO_DIGEST_METHOD_PF g_apfPKIDigestMethod[PKI_HASH_ALGMETHOD_MAX] =
{
    PKI_ResvEvpMd,
    EVP_md5,        
    EVP_sha1,       
    EVP_sha224,     
    EVP_sha256,     
    EVP_sha384,     
    EVP_sha512,     
    EVP_ripemd160   
};


STATIC VOID algo_add_all_algorithms(VOID)
{
    OpenSSL_add_all_algorithms();
    return;
}


const EVP_MD *PKI_ALGO_GetDigestMethodByHashAlgoId(IN PKI_HASH_ALGMETHOD_E enHash)
{
    DBGASSERT (enHash < PKI_HASH_ALGMETHOD_MAX);
    return g_apfPKIDigestMethod[enHash]();
}


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


