/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2015-12-16
* Description: 
* History:     
******************************************************************************/

#ifndef __PKI_UTL_H_
#define __PKI_UTL_H_

#include "openssl/ssl.h"
#include "openssl/pkcs12.h"
#include "openssl/x509v3.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define PKI_CERT_FILENAME_MAX 255UL
#define PKI_CERT_COMMON_NAME_MAX 100UL
#define PKI_CERT_BASIC_CONSTRAINTS_MAX 30UL
#define PKI_KEY_PASSWORD_LENGTH_MAX 20UL
#define PKI_CERT_FRIENDLY_NAME_MAX 100UL

#define PKI_CERTVERSION_MIN                1
#define PKI_CERTVERSION_DEFAULT            2
#define PKI_CERTVERSION_MAX                2
#define PKI_CERTSERIALNUM_DEFAULT          0

#define PKI_CERT_MD5_LENTH      16UL
#define PKI_CERT_SUBJECTNAME_RANDOM_LENTH   PKI_CERT_MD5_LENTH

/* pkcs12格式封装的默认参数 */
#define PKI_NIDCERT_DEFAULT                0
#define PKI_NIDKEY_DEFAULT                 0
#define PKI_MACITER_DEFAULT                (PKCS12_DEFAULT_ITER)
#define PKI_ITERATIONCOUNT_DEFAULT         (PKCS12_DEFAULT_ITER)
#define PKI_KEYUSAGE_MIN                   0
#define PKI_KEYUSAGE_DEFAULT               0
#define PKI_KEYUSAGE_MAX                   255

#define PKI_MODULUS_MIN                    512
#define PKI_MODULUS_DEFAULT                2048
#define PKI_MODULUS_MAX                    2048


typedef enum tagPkiHashAlgorithm
{
    PKI_HASH_ALGMETHOD_RSV,
    PKI_HASH_ALGMETHOD_MD5,
    PKI_HASH_ALGMETHOD_SHA1,
    PKI_HASH_ALGMETHOD_SHA224,
    PKI_HASH_ALGMETHOD_SHA256,
    PKI_HASH_ALGMETHOD_SHA384,
    PKI_HASH_ALGMETHOD_SHA512,
    PKI_HASH_ALGMETHOD_RIPEMD,
    PKI_HASH_ALGMETHOD_MAX
}PKI_HASH_ALGMETHOD_E;

typedef enum tagPkiSigAlgorithm
{
    PKI_SIG_ALGMETHOD_RSV,
    PKI_SIG_ALGMETHOD_RSA,
    PKI_SIG_ALGMETHOD_DSA,
    PKI_SIG_ALGMETHOD_ECDSA,
    PKI_SIG_ALGMETHOD_MAX
}PKI_SIG_ALGMETHOD_E;

typedef struct tagPkiDomainConfigure
{    
    CHAR szCertFileName[PKI_CERT_FILENAME_MAX + 1];          /* 证书文件名称 */
    CHAR szCertSubjectName[PKI_CERT_COMMON_NAME_MAX + 1];          /* 证书subject name */
    CHAR szCertIssuerName[PKI_CERT_COMMON_NAME_MAX + 1];          /* 证书issuer name */
    CHAR szBasicConstraints[PKI_CERT_BASIC_CONSTRAINTS_MAX + 1];
    CHAR szPassword[PKI_KEY_PASSWORD_LENGTH_MAX + 1];
    CHAR szFriendlyname[PKI_CERT_FRIENDLY_NAME_MAX + 1];
    UINT uiSerialNumber;                                /* 证书序列号 */
    UINT uiValidity;                                    /* 证书有效天数 */ 
    UINT uiSigAlgorithm;                                 /* 签名算法 */
    PKI_HASH_ALGMETHOD_E enHash;                         /* hash算法 */
    UINT uiModulus;                                      /* 密钥模数 */
    UINT uiKeyUsage;                                     /* 密钥用途 */
    UINT uiCertVersion;                                  /* 证书版本 */
    UINT uiNidCert;
    UINT uiNidKey;
    UINT uiMacIter;
    UINT uiDefaultIterCount;
}PKI_DOMAIN_CONFIGURE_S;

VOID PKI_InitDftConfig(OUT PKI_DOMAIN_CONFIGURE_S *pstConf);

ULONG PKI_GetSelfSignCertAndKey
(
    IN PKI_DOMAIN_CONFIGURE_S *pstPkiDomainConf, /* 当此参数为NULL时,使用默认配置 */
    OUT X509 **ppstX509Cert, 
    OUT EVP_PKEY **ppstEvpPkey
);

void * PKI_LoadCertByPemFile(char *file, pem_password_cb *cb, void *u);
void * PKI_LoadCertByAsn1File(char *file);
void PKI_FreeX509Cert(void *cert);
void * PKI_LoadPrivateKeyByAsn1File(char *file);
void * PKI_LoadPrivateKeyByPemFile(char *file, pem_password_cb *cb, void *u);
void *PKI_LoadPKCS8PrivateKeybyBuf(char *buf, pem_password_cb *cb, void*u);
int PKI_SaveCertToPemFile(void *cert, char *file);

void X509Name_FreeAllEntry(X509_NAME *name);
int X509Name_AddEntry(X509_NAME *name, char *field, char *value);

int X509Cert_SetIssuer(X509 *cert, X509_NAME *issuer);
int X509Cert_SetPublicKey(X509 *cert, void *pkey);
int X509Cert_Sign(X509 *cert, void *ca_key);
char * X509Cert_GetSubjectName(X509 *pstCert, char *info, int info_size);
void X509Cert_DelExt(X509 *cert, int nid);
typedef BS_WALK_RET_E (*pf_X509Cert_AltNameForEachCB)(ASN1_STRING *alt_name, void *user_data);
void X509Cert_AltNameForEach(X509 *cert, pf_X509Cert_AltNameForEachCB func, void *user_data);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__PKI_UTL_H_*/


