/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-8-18
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
#include <openssl/evp.h>


int AES_Cipher128(UCHAR *key, UCHAR *iv, UCHAR *in, int in_size, UCHAR *out, int out_size, int do_encrypt)
{
    int outlen, finlen;
    EVP_CIPHER_CTX *ctx;

    ctx = EVP_CIPHER_CTX_new();

    if (! EVP_CipherInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv, do_encrypt)) {
        EVP_CIPHER_CTX_free(ctx);
        RETURN(BS_ERR);
    }

    if (!EVP_CipherUpdate(ctx, out, &outlen, in, in_size)) {
        EVP_CIPHER_CTX_free(ctx);
        RETURN(BS_ERR);
    }

    if (!EVP_CipherFinal_ex(ctx, out+outlen, &finlen)) {
        EVP_CIPHER_CTX_free(ctx);
        RETURN(BS_ERR);
    }

    EVP_CIPHER_CTX_free(ctx);

    return outlen + finlen;
}


int AES_Cipher256(UCHAR *key, UCHAR *iv, UCHAR *in, int in_size, UCHAR *out, int out_size, int do_encrypt)
{
    int outlen, finlen;
    EVP_CIPHER_CTX *ctx;

    ctx = EVP_CIPHER_CTX_new();

    if (! EVP_CipherInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv, do_encrypt)) {
        EVP_CIPHER_CTX_free(ctx);
        RETURN(BS_ERR);
    }

    if (!EVP_CipherUpdate(ctx, out, &outlen, in, in_size)) {
        EVP_CIPHER_CTX_free(ctx);
        RETURN(BS_ERR);
    }

    if (!EVP_CipherFinal_ex(ctx, out+outlen, &finlen)) {
        EVP_CIPHER_CTX_free(ctx);
        RETURN(BS_ERR);
    }

    EVP_CIPHER_CTX_free(ctx);

    return outlen + finlen;
}


