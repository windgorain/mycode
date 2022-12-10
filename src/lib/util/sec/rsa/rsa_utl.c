/*================================================================
*   Created by LiXingang: 2018.12.11
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/rsa_utl.h"

static EVP_PKEY * _evp_build_key(EVP_PKEY_CTX *ctx, UINT bits)
{
    EVP_PKEY *pkey = NULL;

    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        return NULL;
    }

    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, bits) <= 0) {
        return NULL;
    }

    /* Generate key */
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0) {
        return NULL;
    }

    return pkey;
}

/* 产生随机秘钥对. bits: 比如2048 */
EVP_PKEY * EVP_BuildKey(int type, UINT bits)
{
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(type, NULL);
    if (! ctx) {
        return NULL;
    }

    EVP_PKEY *pkey = _evp_build_key(ctx, bits);

    EVP_PKEY_CTX_free(ctx);

    return pkey;
}

EVP_PKEY * RSA_BuildKey(UINT bits)
{
    return EVP_RSA_gen(bits);
}

EVP_PKEY * DSA_BuildKey(UINT bits)
{
    return EVP_BuildKey(EVP_PKEY_DSA4, bits);
}

EVP_PKEY * EC_BuildKey(UINT bits)
{
    return EVP_BuildKey(EVP_PKEY_EC, bits);
}

typedef int (*PF_EVP_PKEY_do)(EVP_PKEY_CTX *ctx, UCHAR *out, size_t *outlen, const UCHAR *in, size_t inlen);

static int _evp_do_ctx(EVP_PKEY_CTX *ctx, void *in, int in_len, OUT void *out, int out_size, PF_EVP_PKEY_do func)
{
    size_t out_len;

    if (EVP_PKEY_encrypt_init(ctx) <= 0) {
        RETURN(BS_ERR);
    }

    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
        RETURN(BS_ERR);
	}

    if (EVP_PKEY_encrypt(ctx, NULL, &out_len, in, in_len) <= 0) {
        RETURN(BS_ERR);
    }

    if (out_len > out_size) {
        RETURN(BS_OUT_OF_RANGE);
    }

    if (EVP_PKEY_encrypt(ctx, out, &out_len, in, in_len) <= 0) {
        RETURN(BS_ERR);
	}

    return out_len;
}

static int _evp_do_pkey(EVP_PKEY *key, void *in, int in_len, OUT void *out, int out_size, PF_EVP_PKEY_do func)
{
    EVP_PKEY_CTX *ctx = NULL;

    ctx = EVP_PKEY_CTX_new(key, NULL);
    if (! ctx) {
        RETURN(BS_NO_MEMORY);
	}

    int out_len = _evp_do_ctx(ctx, in, in_len, out, out_size, func);

    EVP_PKEY_CTX_free(ctx);

    return out_len;
}

/* 非对称加密, 返回加密后的数据长度 */
int RSA_Encrypt(IN EVP_PKEY *key, IN void *in, int in_len, OUT void *out, int out_size)
{
    return _evp_do_pkey(key, in, in_len, out, out_size, EVP_PKEY_encrypt);
}

/* 非对称解密, 返回解密后的数据长度 */
int RSA_Decrypt(IN EVP_PKEY *key, IN void *in, int in_len, OUT void *out, int out_size)
{
    return _evp_do_pkey(key, in, in_len, out, out_size, EVP_PKEY_decrypt);
}

