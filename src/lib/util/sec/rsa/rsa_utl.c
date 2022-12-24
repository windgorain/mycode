/*================================================================
*   Created by LiXingang: 2018.12.11
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/rsa_utl.h"

#pragma GCC diagnostic push    // 记录当前的诊断状态
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
 
/* 私钥加密, 返回加密后的数据长度 */
static int _rsa_private_encrypt(RSA *pri_key, IN void *in, int in_len, OUT void *out, int out_size)
{
	int len = RSA_private_encrypt(in_len, in, out, pri_key, RSA_PKCS1_PADDING);
    if (len <= 0) {
        RETURN(BS_ERR);
    }
    return len;
}

/* 公钥解密, 返回解密后的数据长度 */
static int _rsa_public_decrypt(RSA *pub_key, IN void *in, int in_len, OUT void *out, int out_size)
{
	int len = RSA_public_decrypt(in_len, in, out, pub_key, RSA_PKCS1_PADDING);
    if (len <= 0) {
        RETURN(BS_ERR);
    }
    return len;
}

/* 公钥加密, 返回加密后的数据长度 */
static int _rsa_public_encrypt(RSA *pub_key, IN void *in, int in_len, OUT void *out, int out_size)
{
	int len = RSA_public_encrypt(in_len, in, out, pub_key, RSA_PKCS1_PADDING);
    if (len <= 0) {
        RETURN(BS_ERR);
    }
    return len;
}

/* 私钥解密, 返回解密后的数据长度 */
static int _rsa_private_decrypt(RSA *pri_key, IN void *in, int in_len, OUT void *out, int out_size)
{
	int len = RSA_private_decrypt(in_len, in, out, pri_key, RSA_PKCS1_PADDING);
    if (len <= 0) {
        RETURN(BS_ERR);
    }
    return len;
}

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
    return EVP_BuildKey(EVP_PKEY_RSA2, bits);
}

EVP_PKEY * DSA_BuildKey(UINT bits)
{
    return EVP_BuildKey(EVP_PKEY_DSA4, bits);
}

EVP_PKEY * EC_BuildKey(UINT bits)
{
    return EVP_BuildKey(EVP_PKEY_EC, bits);
}

/* 非对称私钥加密, 返回加密后的数据长度 */
int RSA_PrivateEncrypt(IN EVP_PKEY *key, IN void *in, int in_size, OUT void *out, int out_size)
{
    return _rsa_private_encrypt((void*)EVP_PKEY_get0_RSA(key), in, in_size, out, out_size);
}

/* 非对称公钥解密, 返回解密后的数据长度 */
int RSA_PublicDecrypt(IN EVP_PKEY *key, IN void *in, int in_size, OUT void *out, int out_size)
{
    return _rsa_public_decrypt((void*)EVP_PKEY_get0_RSA(key), in, in_size, out, out_size);
}

/* 非对称公钥加密, 返回加密后的数据长度 */
int RSA_PublicEncrypt(IN EVP_PKEY *key, IN void *in, int in_size, OUT void *out, int out_size)
{
    return _rsa_public_encrypt((void*)EVP_PKEY_get0_RSA(key), in, in_size, out, out_size);
}

/* 非对称私钥解密, 返回解密后的数据长度 */
int RSA_PrivateDecrypt(IN EVP_PKEY *key, IN void *in, int in_size, OUT void *out, int out_size)
{
    return _rsa_private_decrypt((void*)EVP_PKEY_get0_RSA(key), in, in_size, out, out_size);
}

#pragma GCC diagnostic pop    // 恢复诊断状

