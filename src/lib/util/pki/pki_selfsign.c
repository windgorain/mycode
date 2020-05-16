/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2015-12-15
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/md5_utl.h"
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

/*****************************************************************************
 生成缺省的密钥对
*****************************************************************************/
static EVP_PKEY *PKI_GenerateResvKey(IN UINT uiKeySize)
{
    (VOID) uiKeySize;
    return NULL;
}

static PKI_GENERATE_KEY_PAIR_PF g_apfPKIGenerateKeyPair[PKI_SIG_ALGMETHOD_MAX] =
{
    PKI_GenerateResvKey,
    PKEY_GenerateRSAKey,
    PKEY_GenerateDSAKey,
    PKI_GenerateResvKey
};

/*****************************************************************************
  Description: 根据ECDSA密钥长度获取NID
        Input: IN UINT uiKeySize  密钥长度
       Output: 无
       Return: INT nid
      Caution: 
*****************************************************************************/
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
            /* 没有长度填充默认值 */
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

/*****************************************************************************
  Description: 对传入的数值做异或运算，将得到的值的十六进制形式作为随机数。
        Input: IN UCHAR aucData[PKI_CERT_MD5_LENTH]    数值数组，长度为PKI_CERT_MD5_LENTH 
       Output: OUT CHAR *pcRandom   随机数字符串
*****************************************************************************/
STATIC VOID PKI_GenerateSubjectRandom(IN UCHAR aucData[PKI_CERT_MD5_LENTH], OUT CHAR *pcRandom)
{
    INT iLen;
    INT *piTmp;
    INT i;
    INT iStepLen;

    pcRandom[0] = '\0';
    
    /* 将aucData中的Hash值(MD5摘要的结果固定为16个字节)的前8个字节和后8个字节进行异或处理
       将得到的数据的十六进制表示形式作为随机数  */
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

/*****************************************************************************
  Description: 设置自签名证书的subject Name和Issuer Name
        Input: IN EVP_PKEY *pstKey 密钥对  
       Output: OUT CHAR *szRandom 从密钥对数据中产生的随机数
       Return: ULONG  ERROR_SUCCESS  成功
                      ERROR_FAILED   失败
*****************************************************************************/
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

    /* 获得密钥数据的MD5值 */
    memset(aucMD, 0, sizeof(aucMD));
    (VOID) MD5_Create(pucData, (ULONG)(LONG) iLen, aucMD);

    free(pucData);

    /* 生成用于SubjectName的随机数 */
    PKI_GenerateSubjectRandom(aucMD, pcRandom);
        
    return ERROR_SUCCESS;
}

/*****************************************************************************
生成指定类型和长度的密钥
        Input: INT iPKIAlgId,   密钥类型，由枚举PKI_SIG_ALGMETHOD_E指定
               UINT uiKeySize   密钥长度
       Return: EVP_PKEY  *     密钥数据
               NULL     生成密钥失败
*****************************************************************************/
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

/*****************************************************************************
  Description: 设置自签名证书的subject Name和Issuer Name
        Input: IN PKI_DOMAIN_CONFIGURE_S *pstPkiDomainConf PKI域CONF结构
               INOUT EVP_PKEY *pstKey 密钥对  
       Output: 无
       Return: ULONG  ERROR_SUCCESS  成功
                      ERROR_FAILED   失败
*****************************************************************************/
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

    /* 将随机数拼装在subject Name后面 */
    ulNameLen = strlen(pstPkiDomainConf->szCertSubjectName);
    snprintf(pstPkiDomainConf->szCertSubjectName + ulNameLen,
             sizeof(pstPkiDomainConf->szCertSubjectName) - ulNameLen,
             "-%s",
             szRandom);
    
    /* 自签名证书的IssuerName和SubjectName是相同的 */
    snprintf(pstPkiDomainConf->szCertIssuerName, sizeof(pstPkiDomainConf->szCertIssuerName), 
             "%s", pstPkiDomainConf->szCertSubjectName);
    
    return ERROR_SUCCESS;
}

/*****************************************************************************
    Func Name: PKI_SetSelfsignedCert
 Date Created: 2010/12/7 
       Author: y06860
  Description: 设置自签名证书属性
        Input: IN PKI_DOMAIN_CONFIGURE_S *pstPkiDomainConf PKI域CONF结构
               IN EVP_PKEY *pstKey   密钥对
               IN X509 *pstX509      证书
       Output: OUT X509 *pstX509      证书
       Return: ERROR_SUCCESS 成功
               ERROR_FAILED  失败
      Caution: 
------------------------------------------------------------------------------
  Modification History                                                      
  DATE        NAME             DESCRIPTION                                  
  --------------------------------------------------------------------------
                                                                            
*****************************************************************************/
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
    
    /* 设置自签名证书version属性 */
    iRet = X509_set_version(pstX509, (LONG)pstPkiDomainConf->uiCertVersion);
    if (1 !=  iRet)
    {
        return ERROR_FAILED;
    }

    /* 设置证书Serial Number属性 */
    pstSerialNumber = X509_get_serialNumber(pstX509);
    iRet = ASN1_INTEGER_set(pstSerialNumber,(LONG)(ULONG)pstPkiDomainConf->uiSerialNumber);
    if (1 != iRet)
    {
        return ERROR_FAILED;
    }
    /* 设置证书有效期 */
    pstTime = X509_get_notBefore(pstX509);
    pstTimeRet = X509_gmtime_adj(pstTime,(LONG)0);
    if (NULL == pstTimeRet)
    {
        return ERROR_FAILED;
    }
    pstTime = X509_get_notAfter(pstX509);
    ulSecsOneDay = (60 * 60) * 24;
    ulTime = (ULONG)pstPkiDomainConf->uiValidity * ulSecsOneDay;

    /* ulTime不会超过0x8FFFFFFF,即ulTime转换为LONG类型后不会出现数据损失 */
    pstTimeRet = X509_gmtime_adj(pstTime, (LONG)ulTime);
    if (NULL == pstTimeRet)
    {
        return ERROR_FAILED;
    }

    /* 设置证书的subject name属性 */
    pstName = X509_get_subject_name(pstX509);
    iRet = X509_NAME_add_entry_by_txt(pstName, "CN", MBSTRING_ASC, (UCHAR *)pstPkiDomainConf->szCertSubjectName,
                                        -1, -1, 0);
    if (1 != iRet)
    {
        return ERROR_FAILED;
    }

    /* 设置证书的issuer name属性 */
    pstName = X509_get_issuer_name(pstX509);
    iRet = X509_NAME_add_entry_by_txt(pstName, "CN", MBSTRING_ASC, (UCHAR *)pstPkiDomainConf->szCertIssuerName, 
                                        -1, -1, 0);
    if (1 != iRet)
    {
        return ERROR_FAILED;
    }
    
    /* 创建扩展属性CA:TRUE */
    pstExtention = X509V3_EXT_conf_nid(NULL, NULL, NID_basic_constraints, pstPkiDomainConf->szBasicConstraints);
    if (NULL == pstExtention)
    {
        return ERROR_FAILED;
    }
    /* 设置扩展属性 */
    iRet = X509_add_ext(pstX509, pstExtention, -1);

    /* 释放扩展属性 */
    X509_EXTENSION_free(pstExtention);

    if (1 != iRet)
    {
        return ERROR_FAILED;
    }

    /* 设置密钥对属性 */
    iRet = X509_set_pubkey(pstX509, pstKey);
    if (1 != iRet)
    {
        return ERROR_FAILED;
    }
    
    return ERROR_SUCCESS;
}

/*****************************************************************************
  Description: 创建自签名证书
        Input: IN PKI_DOMAIN_CONFIGURE_S *pstPkiDomainConf PKI域CONF结构
               IN EVP_PKEY *pstKey 密钥对  
       Output: 无
       Return: X509 *,成功返回指向X509结构的指针
               失败返回NULL
*****************************************************************************/
STATIC X509 *PKI_GenerateSelfsignedCert(IN PKI_DOMAIN_CONFIGURE_S *pstPkiDomainConf, IN EVP_PKEY *pstKey)
{
    INT iRet;
    ULONG ulResult;
    X509 *pstX509;
    const EVP_MD *pstEvpMd;
    ULONG ulRet;

    /* 处理自签名证书的Subject名称和issuer名称,将密钥的Hash值作为随机数加到名称字符串中 */
    ulRet = PKI_SetSelfsignedCertSubjectAndIssuer(pstKey, pstPkiDomainConf);
    if (ERROR_SUCCESS != ulRet)
    {
        return NULL;
    }
    
    /* 申请证书结构体资源 */
    pstX509 = X509_new();
    if (NULL == pstX509)
    {
        return NULL;
    }

    /* 设置自签名证书属性 */
    ulResult = PKI_SetSelfsignedCert(pstPkiDomainConf, pstKey, pstX509);
    if (ERROR_SUCCESS == ulResult)
    {
        iRet = 0;
        pstEvpMd = PKI_ALGO_GetDigestMethodByHashAlgoId(pstPkiDomainConf->enHash);
        if (NULL != pstEvpMd)
        {
            /* 对证书进行签名 */
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

/*****************************************************************************
  Description: 将自签名证书以PKCS12格式写入设备文件中
        Input: IN PKI_DOMAIN_CONFIGURE_S *pstPkiDomainConf PKI域CONF结构
               IN X509 *pstX509Cert      证书
               IN EVP_PKEY * pstEvpPkey  密钥对
*****************************************************************************/
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
        /* 载入所有算法 */
        PKI_ALGO_add_all_algorithms();

        /* 生成PKCS12格式证书 */
        pstPKCS12 = PKCS12_create(pstPkiDomainConf->szPassword, pstPkiDomainConf->szFriendlyname, 
                                    pstEvpPkey, pstX509Cert, 
                                    NULL, (INT)pstPkiDomainConf->uiNidKey, 
                                    (INT)pstPkiDomainConf->uiNidCert, 
                                    (INT)pstPkiDomainConf->uiDefaultIterCount,
                                    (INT)pstPkiDomainConf->uiMacIter,
                                    (INT)pstPkiDomainConf->uiKeyUsage);
        if (NULL != pstPKCS12)
        {
            /* 写入文件 */
            iRet = i2d_PKCS12_fp(pstFile, pstPKCS12);
            if (1 != iRet)
            {
                ulRet = ERROR_FAILED;
            }
            
            /* 释放PKCS12资源 */
            PKCS12_free(pstPKCS12);
        }
        else
        {
            ulRet = ERROR_FAILED;
        }

        fclose(pstFile);

        if (ERROR_SUCCESS != ulRet)
        {
            /* 如果保存证书格式失败了,删除之前创建的空文件,不删除创建的目录结构 */
            FILE_DelFile(szDevCertFileName);
            
        }        
    }
    
    return ulRet;
}

/*****************************************************************************
 创建自签名证书以及密钥对
*****************************************************************************/
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
    
    /* 生成密钥对 */
    pstKey = PKI_GeneratePKEY((INT) pstPkiDomainConf->uiSigAlgorithm, pstPkiDomainConf->uiModulus);
    if (NULL == pstKey)
    {
        return ERROR_FAILED;
    }
    /* 生成X509格式自签名证书 */
    pstX509 = PKI_GenerateSelfsignedCert(pstPkiDomainConf, pstKey);
    if (NULL == pstX509)
    {
        EVP_PKEY_free(pstKey);
        pstKey = NULL;
        ulResult = ERROR_FAILED;
    }
    else
    {
        /* 将生成的证书写入文件,不必关心本次是否写入成功 */
        (VOID) PKI_WritePkcs12SelfSignedCert(pstPkiDomainConf, pstX509, pstKey);

        ulResult = ERROR_SUCCESS;
    }

    /* 统一填充输出参数并返回错误码 */
    *ppstX509Cert = pstX509;
    *ppstEvpPkey = pstKey;
    
    return ulResult;
}

/*****************************************************************************
  Description: 解析PKCS12自签名证书文件
        Input: IN const PKI_DOMAIN_CONFIGURE_S *pstPkiDomainConf  PKI配置CONF结构
               IN FILE *pstFile          文件指针
       Output: OUT X509 **ppstX509Cert   X509类型证书     
               OUT EVP_PKEY **ppstEvpPkey   密钥对
       Return: ULONG ERROR_SUCCESS ,自签名证书以及密钥对成功
               ULONG ERROR_FAILED ,自签名证书以及密钥对失败
*****************************************************************************/
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

    /* 载入所有算法 */
    PKI_ALGO_add_all_algorithms();
    
    /* 从文件中读取PKCS12证书内容 */
    pstPkcs12 = d2i_PKCS12_fp(pstFile, NULL);       
    if ( NULL == pstPkcs12 )
    {
        return ERROR_FAILED;
    }
    /* 从PKCS12结构资源中获取证书的密钥对和证书 */
    iParseRet = PKCS12_parse(pstPkcs12, pstPkiDomainConf->szPassword, &pstKey, &pstCert, NULL);
    
    /* 释放PKCS12证书资源 */
    PKCS12_free(pstPkcs12);

    if (1 != iParseRet)
    {
        return ERROR_FAILED;
    }

    /* 填充输出参数 */
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

/*****************************************************************************
  Description: 获取自签名证书以及密钥对
        Input: IN PKI_DOMAIN_CONFIGURE_S *pstPkiDomainConf PKI域CONF结构 
       Output: OUT X509 **ppstX509Cert   X509类型证书     
               OUT EVP_PKEY **ppstEvpPkey   密钥对
       Return: ULONG ERROR_SUCCESS ,自签名证书以及密钥对成功
               ULONG ERROR_FAILED ,自签名证书以及密钥对失败
*****************************************************************************/
ULONG PKI_GetSelfSignCertAndKey
(
    IN PKI_DOMAIN_CONFIGURE_S *pstPkiDomainConf, /* 当此参数为NULL时,使用默认配置 */
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

    /* 检查文件是否存在 */
    bIsExist = FILE_IsFileExist(pcCertFileName);
    if (BOOL_TRUE == bIsExist)
    {
        pstFile = FILE_Open(pcCertFileName, FALSE, "rb");  
        if (NULL != pstFile)
        {
            /* 从PKCS12证书中读取证书以及密钥对 */
            ulErrcode = PKI_ParsePkcs12SelfsignedCert(pstPkiDomainConf, pstFile, ppstX509Cert, ppstEvpPkey);
            FILE_Close(pstFile);
        }
    }
    if (ERROR_SUCCESS != ulErrcode)
    {
        /* 重新生成自签名证书以及密钥对 */
        ulErrcode = PKI_CreateSelfsignedCertAndKey(pstPkiDomainConf, ppstX509Cert, ppstEvpPkey);
    }

    return ulErrcode;
}


