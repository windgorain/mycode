/*================================================================
*   Created by LiXingang: 2018.12.11
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/rsa_utl.h"

RSA *RSA_BuildKey(int bits)
{
    RSA *rsa = RSA_new();
    BIGNUM* e = BN_new();

    if (rsa == NULL || e == NULL)
        goto err;

    /* 设置随机数长度 */
    BN_set_word(e, 65537);

    /* 生成RSA密钥对 */
    if (! RSA_generate_key_ex(rsa, bits, e, NULL)) {
        goto err;
    }

    return rsa;

 err:
    BN_free(e);
    RSA_free(rsa);
    return 0;
}

/* 私钥加密, 返回加密后的数据长度 */
int RSA_PrivateEncrypt(IN RSA *pri_key, IN void *in, int in_len, OUT void *out, int out_size)
{
    int out_len;
    int len;

	out_len =  RSA_size(pri_key);

    if (out_size < out_len) {
        BS_DBGASSERT(0);
        RETURN(BS_OUT_OF_RANGE);
    }

	memset(out, 0, out_len);
 
	len = RSA_private_encrypt(in_len, in, out, pri_key, RSA_PKCS1_PADDING);
    if (len <= 0) {
        RETURN(BS_ERR);
    }

    return len;
}

/* 公钥解密, 返回解密后的数据长度 */
int RSA_PublicDecrypt(IN RSA *pub_key, IN void *in, int in_len, OUT void *out, int out_size)
{
    int out_len;

	out_len =  RSA_size(pub_key);

    if (out_size < out_len) {
        BS_DBGASSERT(0);
        RETURN(BS_OUT_OF_RANGE);
    }

	memset(out, 0, out_len);
 
	return RSA_public_decrypt(in_len, in, out, pub_key, RSA_PKCS1_PADDING);
}

/* 公钥加密, 返回加密后的数据长度 */
int RSA_PublicEncrypt(IN RSA *pub_key, IN void *in, int in_len, OUT void *out, int out_size)
{
    int out_len;

	out_len =  RSA_size(pub_key);
    if (out_len > out_size) {
        BS_DBGASSERT(0);
        RETURN(BS_OUT_OF_RANGE);
    }

    memset(out, 0, out_len);

	return RSA_public_encrypt(in_len, in, out, pub_key, RSA_PKCS1_PADDING);
}

/* 私钥解密, 返回解密后的数据长度 */
int RSA_PrivateDecrypt(IN RSA *pri_key, IN void *in, int in_len, OUT void *out, int out_size)
{
    int out_len;

	out_len =  RSA_size(pri_key);
    if (out_len > out_size) {
        BS_DBGASSERT(0);
        RETURN(BS_OUT_OF_RANGE);
    }

	memset(out, 0, out_len);

	return RSA_private_decrypt(in_len, in, out, pri_key, RSA_PKCS1_PADDING);
}

