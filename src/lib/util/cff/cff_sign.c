/*================================================================
*   Created by LiXingang: 2018.12.12
*   Description: 为CFF做签名和验证
*
================================================================*/
#include  "bs.h"

#include "utl/string_utl.h"
#include "utl/data2hex_utl.h"
#include "utl/rsa_utl.h"
#include "utl/md5_utl.h"
#include "utl/cff_utl.h"
#include "utl/cff_sign.h"

#define CFF_SIGN_PROP_NAME "_signature_"

int cffsign_CalcSignature(CFF_HANDLE hCff, IN char *tag_name, UINT flag, OUT UCHAR *md5_data)
{
    char *key, *value;
    HSTRING str;
    BS_STATUS eRet = BS_OK;

    str = STRING_Create();
    if (NULL == str) {
        RETURN(BS_NO_MEMORY);
    }

    if (flag & CFF_SIGN_FLAG_INCLUDE_TAGNAME) {
        eRet |= STRING_CatFromBuf(str, "[");
        eRet |= STRING_CatFromBuf(str, tag_name);
        eRet |= STRING_CatFromBuf(str, "]");
    }

    CFF_SCAN_PROP_START(hCff, tag_name, key, value) {
        if (strcmp(key, CFF_SIGN_PROP_NAME) == 0) {
            continue;
        }
        eRet |= STRING_CatFromBuf(str, key);
        eRet |= STRING_CatFromBuf(str, ":");
        eRet |= STRING_CatFromBuf(str, value);
        eRet |= STRING_CatFromBuf(str, ",");
    }CFF_SCAN_END();

    if (BS_OK == eRet) {
        MD5_Create(STRING_GetBuf(str), STRING_GetLength(str), md5_data);
    }

    STRING_Delete(str);

    return 0;
}

static int cffsign_GetSignature(IN CFF_HANDLE hCff, IN char *tag_name, OUT UCHAR *buf, IN UINT buf_size)
{
    int len;
    char *signature;

    if (BS_OK != CFF_GetPropAsString(hCff, tag_name, CFF_SIGN_PROP_NAME, &signature)) {
        RETURN(BS_NO_SUCH);
    }

    len = strlen(signature);
    if (len == 0) {
        RETURN(BS_NO_SUCH);
    }

    if (len & 0x1) {
        RETURN(BS_ERR);
    }

    len = len>>1;
    if (len >= buf_size) {
        RETURN(BS_OUT_OF_RANGE);
    }

    DH_HexString2Data(signature, buf);

    return len;
}

static int cffsign_PublicDecryptSignature(CFF_HANDLE hcff, char *tag_name, EVP_PKEY *pub_key, UCHAR *md5_data)
{
    int len;
    int dec_len;
    UCHAR buf[513];
    char dec[513];

    if (pub_key == NULL) {
        return -1;
    }

    len = cffsign_GetSignature(hcff, tag_name, buf, sizeof(buf));
    if (len <= 0) {
        RETURN(BS_ERR);
    }

    dec_len = RSA_PublicDecrypt(pub_key, buf, len, dec, sizeof(dec));
    if (dec_len != MD5_LEN) {
        RETURN(BS_ERR);
    }

    memcpy(md5_data, dec, MD5_LEN);

    return 0;
}

static int cffsign_PrivateDecryptSignature(IN CFF_HANDLE hcff, IN char *tag_name, IN EVP_PKEY *pri_key, OUT UCHAR *md5_data)
{
    int len;
    int dec_len;
    UCHAR buf[513];
    char dec[513];

    len = cffsign_GetSignature(hcff, tag_name, buf, sizeof(buf));
    if (len <= 0) {
        RETURN(BS_ERR);
    }

    dec_len = RSA_PrivateDecrypt(pri_key, buf, len, dec, sizeof(dec));
    if (dec_len != MD5_LEN) {
        RETURN(BS_ERR);
    }

    memcpy(md5_data, dec, MD5_LEN);

    return 0;
}

static int cffsign_SetSignProp(IN CFF_HANDLE hCff, IN char *tag_name, IN void *buf, IN UINT buf_len)
{
    char *hex;
    int ret;

    hex = MEM_Malloc(buf_len * 2 + 1);
    if (NULL == hex) {
        RETURN(BS_NO_MEMORY);
    }

    DH_Data2HexString(buf, buf_len, hex);

    ret = CFF_SetPropAsString(hCff, tag_name, CFF_SIGN_PROP_NAME, hex);
    MEM_Free(hex);

    return ret;
}

static int cffsign_PrivateSign(IN CFF_HANDLE hCff, IN char *tag_name, IN EVP_PKEY *pri_key, OUT UCHAR *md5_data)
{
    int len;
    char buf[513];

    len = RSA_PrivateEncrypt(pri_key, md5_data, MD5_LEN, buf, sizeof(buf));
    if (len <= 0) {
        RETURN(BS_ERR);
    }

    return cffsign_SetSignProp(hCff, tag_name, buf, len);
}

static int cffsign_PublicSign(IN CFF_HANDLE hCff, IN char *tag_name, IN EVP_PKEY *pub_key, OUT UCHAR *md5_data)
{
    int len;
    char buf[513];

    len = RSA_PublicEncrypt(pub_key, md5_data, MD5_LEN, buf, sizeof(buf));
    if (len < 0) {
        RETURN(BS_ERR);
    }

    return cffsign_SetSignProp(hCff, tag_name, buf, len);
}

int CFFSign_PrivateSign(IN CFF_HANDLE hCff, IN char *tag_name, IN void *pri_key, UINT flag)
{
    UCHAR md5_data[MD5_LEN];

    if (0 != cffsign_CalcSignature(hCff, tag_name, flag, md5_data)) {
        return -1;
    }

    return cffsign_PrivateSign(hCff, tag_name, pri_key, md5_data);
}

int CFFSign_PublicVerify(CFF_HANDLE hCff, IN char *tag_name, IN EVP_PKEY *pub_key, UINT flag)
{
    UCHAR md5_data1[MD5_LEN];
    UCHAR md5_data2[MD5_LEN];

    if (0 != cffsign_CalcSignature(hCff, tag_name, flag, md5_data1)) {
        return -1;
    }

    if (0 != cffsign_PublicDecryptSignature(hCff, tag_name, pub_key, md5_data2)) {
        return -1;
    }

    if (memcmp(md5_data1, md5_data2, MD5_LEN) != 0) {
        return -1;
    }

    return 0;
}

int CFFSign_PublicSign(IN CFF_HANDLE hCff, IN char *tag_name, IN void *pub_key, UINT flag)
{
    UCHAR md5_data[MD5_LEN];

    if (0 != cffsign_CalcSignature(hCff, tag_name, flag, md5_data)) {
        return -1;
    }

    return cffsign_PublicSign(hCff, tag_name, pub_key, md5_data);
}

int CFFSign_PrivateVerify(CFF_HANDLE hCff, IN char *tag_name, IN void *pri_key, UINT flag)
{
    UCHAR md5_data1[MD5_LEN];
    UCHAR md5_data2[MD5_LEN];

    if (0 != cffsign_CalcSignature(hCff, tag_name, flag, md5_data1)) {
        return -1;
    }

    if (0 != cffsign_PrivateDecryptSignature(hCff, tag_name, pri_key, md5_data2)) {
        return -1;
    }

    if (memcmp(md5_data1, md5_data2, MD5_LEN) != 0) {
        return -1;
    }

    return 0;
}
