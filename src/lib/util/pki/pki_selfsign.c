/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2015-12-15
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/md5_utl.h"
#include "utl/rsa_utl.h"
#include "utl/file_utl.h"
#include "utl/txt_utl.h"
#include "utl/pki_utl.h"
#include "openssl/ssl.h"
#include "openssl/pkcs12.h"
#include "openssl/x509v3.h"

#include "pki_algo.h"
#include "pkey_local.h"
#include "pki_pubkey.h"

typedef EVP_PKEY *(*PKI_GENERATE_KEY_PAIR_PF)(IN UINT uiKeySize);

static PKI_DOMAIN_CONFIGURE_S g_pkiSelfsignDftConfig = 
{
    "https-server.p12",
    "HTTPS-Self-Signed-Certificate",
    "",
    "CA:FALSE",
    "",
    "",
    0,
    7300,
    1,
    
};

static PKI_DOMAIN_CONFIGURE_S * pki_GetDftConfig()
{
    if (g_pkiSelfsignDftConfig.szCertFileName[0] == '\0')
    {
        if (g_pkiSelfsignDftConfig.szCertFileName[0] == '\0')
        {
            PKI_InitDftConfig(&g_pkiSelfsignDftConfig);
        }
    }

    return &g_pkiSelfsignDftConfig;
}


static EVP_PKEY * pki_generate_resv_key(IN UINT uiKeySize)
{
    (VOID) uiKeySize;
    return NULL;
}

static PKI_GENERATE_KEY_PAIR_PF g_apfPKIGenerateKeyPair[PKI_SIG_ALGMETHOD_MAX] =
{
    pki_generate_resv_key,
    RSA_BuildKey,
    DSA_BuildKey,
    pki_generate_resv_key
};


static INT _pki_GetECKeyNidByKeySize(IN UINT uiKeySize)
{
    INT iNid = 0;
    
    switch (uiKeySize)
    {
        case PKI_PKEY_EC_P192_LEN:
        {
            iNid = NID_X9_62_prime192v1;
            break;
        }
        case PKI_PKEY_EC_P256_LEN:
        {
            iNid = NID_X9_62_prime256v1;
            break;
        }
        case PKI_PKEY_EC_P384_LEN:
        {
            iNid = NID_secp384r1;
            break;
        }
        case 0:
        {
            
            iNid = NID_X9_62_prime256v1;
            break;
        }
        default:
        {
            DBGASSERT(0);
            break;
        }
    }

    return iNid;
}


STATIC VOID PKI_GenerateSubjectRandom(IN UCHAR aucData[PKI_CERT_MD5_LENTH], OUT CHAR *pcRandom)
{
    INT iLen;
    INT *piTmp;
    INT i;
    INT iStepLen;

    pcRandom[0] = '\0';
    
    
    iLen = PKI_CERT_MD5_LENTH/8;
    piTmp = (INT *) aucData;
    for (i = 0; i < iLen; i++)
    {
        *(piTmp + i) = *(piTmp + i) ^ *(piTmp + i + iLen);
        iStepLen = 8 * i;
        snprintf(pcRandom + iStepLen, 
            PKI_CERT_SUBJECTNAME_RANDOM_LENTH + 1 - (ULONG)(LONG) iStepLen, 
            "%08x", 
            *(piTmp + i));
    }

    return;
}


STATIC ULONG PKI_GenerateRandomFromPKEY(IN EVP_PKEY *pstKey, OUT CHAR *pcRandom)
{
    INT iLen;
    UCHAR *pucData;
    UCHAR aucMD[PKI_CERT_MD5_LENTH];

    DBGASSERT(NULL != pstKey);
    DBGASSERT(NULL != pcRandom);
    
    pucData = NULL;
    iLen = i2d_PrivateKey(pstKey, &pucData);
    if (iLen <= 0)
    {
        return ERROR_FAILED;
    }

    
    memset(aucMD, 0, sizeof(aucMD));
    (VOID) MD5_Create(pucData, (ULONG)(LONG) iLen, aucMD);

    free(pucData);

    
    PKI_GenerateSubjectRandom(aucMD, pcRandom);
        
    return ERROR_SUCCESS;
}


EVP_PKEY *PKI_GeneratePKEY(INT iPKIAlgId, UINT uiKeySize)
{
    INT iNid;
    
    if (iPKIAlgId < PKI_SIG_ALGMETHOD_ECDSA)
    {        
        return g_apfPKIGenerateKeyPair[iPKIAlgId](uiKeySize);
    }
    if (PKI_SIG_ALGMETHOD_ECDSA == iPKIAlgId)
    {
        iNid = _pki_GetECKeyNidByKeySize(uiKeySize);
        if (iNid) {
            return PKEY_GenerateECKey(iNid);
        }
    }
    return NULL;
}


STATIC ULONG PKI_SetSelfsignedCertSubjectAndIssuer
(
    IN EVP_PKEY *pstKey, 
    INOUT PKI_DOMAIN_CONFIGURE_S *pstPkiDomainConf
)
{
    ULONG ulRet;
    ULONG ulNameLen;
    CHAR szRandom[PKI_CERT_SUBJECTNAME_RANDOM_LENTH + 1];

    DBGASSERT(NULL != pstKey);
    DBGASSERT(NULL != pstPkiDomainConf);
    
    szRandom[0] = '\0';
    ulRet = PKI_GenerateRandomFromPKEY(pstKey,szRandom);
    if (ERROR_SUCCESS != ulRet)
    {
        return ERROR_FAILED;
    }

    
    ulNameLen = strlen(pstPkiDomainConf->szCertSubjectName);
    snprintf(pstPkiDomainConf->szCertSubjectName + ulNameLen,
             sizeof(pstPkiDomainConf->szCertSubjectName) - ulNameLen,
             "-%s",
             szRandom);
    
    
    snprintf(pstPkiDomainConf->szCertIssuerName, sizeof(pstPkiDomainConf->szCertIssuerName), 
             "%s", pstPkiDomainConf->szCertSubjectName);
    
    return ERROR_SUCCESS;
}


STATIC ULONG PKI_SetSelfsignedCert
(
    IN PKI_DOMAIN_CONFIGURE_S *pstPkiDomainConf, 
    IN EVP_PKEY *pstKey, 
    INOUT X509 *pstX509
)
{
    INT iRet;
    ULONG ulSecsOneDay;
    ULONG ulTime;
    ASN1_INTEGER *pstSerialNumber;
    ASN1_TIME *pstTime;
    ASN1_TIME *pstTimeRet;   
    X509_NAME *pstName = NULL;
    X509_EXTENSION *pstExtention = NULL;
    
    
    iRet = X509_set_version(pstX509, (LONG)pstPkiDomainConf->uiCertVersion);
    if (1 !=  iRet)
    {
        return ERROR_FAILED;
    }

    
    pstSerialNumber = X509_get_serialNumber(pstX509);
    iRet = ASN1_INTEGER_set(pstSerialNumber,(LONG)(ULONG)pstPkiDomainConf->uiSerialNumber);
    if (1 != iRet)
    {
        return ERROR_FAILED;
    }
    
    pstTime = X509_get_notBefore(pstX509);
    pstTimeRet = X509_gmtime_adj(pstTime,(LONG)0);
    if (NULL == pstTimeRet)
    {
        return ERROR_FAILED;
    }
    pstTime = X509_get_notAfter(pstX509);
    ulSecsOneDay = (60 * 60) * 24;
    ulTime = (ULONG)pstPkiDomainConf->uiValidity * ulSecsOneDay;

    
    pstTimeRet = X509_gmtime_adj(pstTime, (LONG)ulTime);
    if (NULL == pstTimeRet)
    {
        return ERROR_FAILED;
    }

    
    pstName = X509_get_subject_name(pstX509);
    iRet = X509_NAME_add_entry_by_txt(pstName, "CN", MBSTRING_ASC, (UCHAR *)pstPkiDomainConf->szCertSubjectName,
                                        -1, -1, 0);
    if (1 != iRet)
    {
        return ERROR_FAILED;
    }

    
    pstName = X509_get_issuer_name(pstX509);
    iRet = X509_NAME_add_entry_by_txt(pstName, "CN", MBSTRING_ASC, (UCHAR *)pstPkiDomainConf->szCertIssuerName, 
                                        -1, -1, 0);
    if (1 != iRet)
    {
        return ERROR_FAILED;
    }
    
    
    pstExtention = X509V3_EXT_conf_nid(NULL, NULL, NID_basic_constraints, pstPkiDomainConf->szBasicConstraints);
    if (NULL == pstExtention)
    {
        return ERROR_FAILED;
    }
    
    iRet = X509_add_ext(pstX509, pstExtention, -1);

    
    X509_EXTENSION_free(pstExtention);

    if (1 != iRet)
    {
        return ERROR_FAILED;
    }

    
    iRet = X509_set_pubkey(pstX509, pstKey);
    if (1 != iRet)
    {
        return ERROR_FAILED;
    }
    
    return ERROR_SUCCESS;
}


STATIC X509 *PKI_GenerateSelfsignedCert(IN PKI_DOMAIN_CONFIGURE_S *pstPkiDomainConf, IN EVP_PKEY *pstKey)
{
    INT iRet;
    ULONG ulResult;
    X509 *pstX509;
    const EVP_MD *pstEvpMd;
    ULONG ulRet;

    
    ulRet = PKI_SetSelfsignedCertSubjectAndIssuer(pstKey, pstPkiDomainConf);
    if (ERROR_SUCCESS != ulRet)
    {
        return NULL;
    }
    
    
    pstX509 = X509_new();
    if (NULL == pstX509)
    {
        return NULL;
    }

    
    ulResult = PKI_SetSelfsignedCert(pstPkiDomainConf, pstKey, pstX509);
    if (ERROR_SUCCESS == ulResult)
    {
        iRet = 0;
        pstEvpMd = PKI_ALGO_GetDigestMethodByHashAlgoId(pstPkiDomainConf->enHash);
        if (NULL != pstEvpMd)
        {
            
            iRet = X509_sign(pstX509, pstKey, pstEvpMd);
        }
        
        if (0 == iRet)
        {
            ulResult = ERROR_FAILED;
        }
    }

    if (ERROR_SUCCESS != ulResult)
    {
        X509_free(pstX509);
        pstX509 = NULL;
    }

    return pstX509;    
}


STATIC ULONG PKI_WritePkcs12SelfSignedCert
(
    IN PKI_DOMAIN_CONFIGURE_S *pstPkiDomainConf, 
    IN X509 *pstX509Cert, 
    IN EVP_PKEY *pstEvpPkey
)
{
    INT iRet;
    ULONG ulRet = ERROR_SUCCESS;
    FILE *pstFile;
    PKCS12 *pstPKCS12;
    CHAR *szDevCertFileName = pstPkiDomainConf->szCertFileName;

    pstFile = FILE_Open(szDevCertFileName, TRUE, "w+");
    if (NULL != pstFile)
    {
        
        PKI_ALGO_add_all_algorithms();

        
        pstPKCS12 = PKCS12_create(pstPkiDomainConf->szPassword, pstPkiDomainConf->szFriendlyname, 
                                    pstEvpPkey, pstX509Cert, 
                                    NULL, (INT)pstPkiDomainConf->uiNidKey, 
                                    (INT)pstPkiDomainConf->uiNidCert, 
                                    (INT)pstPkiDomainConf->uiDefaultIterCount,
                                    (INT)pstPkiDomainConf->uiMacIter,
                                    (INT)pstPkiDomainConf->uiKeyUsage);
        if (NULL != pstPKCS12)
        {
            
            iRet = i2d_PKCS12_fp(pstFile, pstPKCS12);
            if (1 != iRet)
            {
                ulRet = ERROR_FAILED;
            }
            
            
            PKCS12_free(pstPKCS12);
        }
        else
        {
            ulRet = ERROR_FAILED;
        }

        fclose(pstFile);

        if (ERROR_SUCCESS != ulRet)
        {
            
            FILE_DelFile(szDevCertFileName);
            
        }        
    }
    
    return ulRet;
}


STATIC ULONG PKI_CreateSelfsignedCertAndKey
(
    IN PKI_DOMAIN_CONFIGURE_S *pstPkiDomainConf, 
    OUT X509 **ppstX509Cert, 
    OUT EVP_PKEY **ppstEvpPkey
)
{
    ULONG ulResult;
    X509 *pstX509;
    EVP_PKEY *pstKey;

    DBGASSERT(NULL != ppstX509Cert);
    DBGASSERT(NULL != ppstEvpPkey);
    
    
    pstKey = PKI_GeneratePKEY((INT) pstPkiDomainConf->uiSigAlgorithm, pstPkiDomainConf->uiModulus);
    if (NULL == pstKey)
    {
        return ERROR_FAILED;
    }
    
    pstX509 = PKI_GenerateSelfsignedCert(pstPkiDomainConf, pstKey);
    if (NULL == pstX509)
    {
        EVP_PKEY_free(pstKey);
        pstKey = NULL;
        ulResult = ERROR_FAILED;
    }
    else
    {
        
        (VOID) PKI_WritePkcs12SelfSignedCert(pstPkiDomainConf, pstX509, pstKey);

        ulResult = ERROR_SUCCESS;
    }

    
    *ppstX509Cert = pstX509;
    *ppstEvpPkey = pstKey;
    
    return ulResult;
}


STATIC ULONG PKI_ParsePkcs12SelfsignedCert
(
    IN const PKI_DOMAIN_CONFIGURE_S *pstPkiDomainConf, 
    IN FILE *pstFile, 
    OUT X509 **ppstX509Cert, 
    OUT EVP_PKEY **ppstEvpPkey
)
{
    INT iParseRet;
    PKCS12 *pstPkcs12;
    X509 *pstCert = NULL;
    EVP_PKEY *pstKey = NULL;

    
    PKI_ALGO_add_all_algorithms();
    
    
    pstPkcs12 = d2i_PKCS12_fp(pstFile, NULL);       
    if ( NULL == pstPkcs12 )
    {
        return ERROR_FAILED;
    }
    
    iParseRet = PKCS12_parse(pstPkcs12, pstPkiDomainConf->szPassword, &pstKey, &pstCert, NULL);
    
    
    PKCS12_free(pstPkcs12);

    if (1 != iParseRet)
    {
        return ERROR_FAILED;
    }

    
    *ppstX509Cert = pstCert;
    *ppstEvpPkey = pstKey;

    return ERROR_SUCCESS;
}

VOID PKI_InitDftConfig(OUT PKI_DOMAIN_CONFIGURE_S *pstConf)
{
    memset(pstConf, 0, sizeof(PKI_DOMAIN_CONFIGURE_S));

    strlcpy(pstConf->szCertFileName, "server_cert.p12", sizeof(pstConf->szCertFileName));
    strlcpy(pstConf->szCertSubjectName, "Self-Signed-Certificate", sizeof(pstConf->szCertSubjectName));
    strlcpy(pstConf->szBasicConstraints, "CA:FALSE", sizeof(pstConf->szBasicConstraints));
    pstConf->uiValidity = 7300;
    pstConf->uiSigAlgorithm = PKI_SIG_ALGMETHOD_RSA;
    pstConf->enHash = PKI_HASH_ALGMETHOD_SHA256;
    pstConf->uiModulus = PKI_MODULUS_DEFAULT;
    pstConf->uiCertVersion = PKI_CERTVERSION_DEFAULT;
    pstConf->uiMacIter = PKI_MACITER_DEFAULT;
    pstConf->uiDefaultIterCount = PKI_ITERATIONCOUNT_DEFAULT;
}


ULONG PKI_GetSelfSignCertAndKey
(
    IN PKI_DOMAIN_CONFIGURE_S *pstPkiDomainConf, 
    OUT X509 **ppstX509Cert, 
    OUT EVP_PKEY **ppstEvpPkey
)
{
    ULONG ulErrcode = ERROR_FAILED;
    BOOL_T bIsExist;
    FILE *pstFile;
    CHAR *pcCertFileName;

    if (NULL == pstPkiDomainConf)
    {
        pstPkiDomainConf = pki_GetDftConfig();
    }

    pcCertFileName = pstPkiDomainConf->szCertFileName;

    
    bIsExist = FILE_IsFileExist(pcCertFileName);
    if (BOOL_TRUE == bIsExist)
    {
        pstFile = FILE_Open(pcCertFileName, FALSE, "rb");  
        if (NULL != pstFile)
        {
            
            ulErrcode = PKI_ParsePkcs12SelfsignedCert(pstPkiDomainConf, pstFile, ppstX509Cert, ppstEvpPkey);
            FILE_Close(pstFile);
        }
    }
    if (ERROR_SUCCESS != ulErrcode)
    {
        
        ulErrcode = PKI_CreateSelfsignedCertAndKey(pstPkiDomainConf, ppstX509Cert, ppstEvpPkey);
    }

    return ulErrcode;
}


