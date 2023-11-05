/*================================================================
*   Created：2018.11.02
*   Description：
*
================================================================*/
#include "bs.h"

#include "utl/file_utl.h"
#include "utl/txt_utl.h"
#include "utl/ssl_utl.h"
#include "utl/pki_utl.h"
#include "utl/fake_cert.h"
#include "openssl/ssl.h"
#include "openssl/pkcs12.h"
#include "openssl/x509v3.h"

int FakeCert_init(FAKECERT_CTRL_S *ctrl, char *ca_certfile, char *ca_keyfile, char *self_keyfile, pem_password_cb *cb, void *u)
{
    ctrl->ca_cert = PKI_LoadCertByPemFile(ca_certfile, cb, u);
    ctrl->ca_key = PKI_LoadPrivateKeyByPemFile(ca_keyfile, cb, u);
    ctrl->self_key = PKI_LoadPrivateKeyByPemFile(self_keyfile, cb, u);

    if ((ctrl->ca_cert == NULL) || (ctrl->ca_key == NULL) || (ctrl->self_key == NULL)) {
        FakeCdrt_Finit(ctrl);
        return -1;
    }

    return 0;
}

int FakeCert_Init_From_Buf(FAKECERT_CTRL_S *ctrl, char *ca_certfile,  char *ca_pkcs8_key, char *self_keyfile,pem_password_cb *cb, void *u)
{
    ctrl->ca_cert = PKI_LoadCertByPemFile(ca_certfile, cb, u);
    ctrl->ca_key = PKI_LoadPKCS8PrivateKeybyBuf(ca_pkcs8_key, cb, u);
    ctrl->self_key = PKI_LoadPrivateKeyByPemFile(self_keyfile, cb, u);

    if ((ctrl->ca_cert == NULL) || (ctrl->ca_key == NULL) || (ctrl->self_key == NULL)) {
        FakeCdrt_Finit(ctrl);
        return -1;
    }

    return 0;
}


void FakeCdrt_Finit(FAKECERT_CTRL_S *ctrl)
{
    if (NULL != ctrl->ca_cert) {
        X509_free(ctrl->ca_cert);
        ctrl->ca_cert = NULL;
    }

    if (NULL != ctrl->ca_key) {
        EVP_PKEY_free(ctrl->ca_key);
        ctrl->ca_key = NULL;
    }

    if (NULL != ctrl->self_key) {
        EVP_PKEY_free(ctrl->self_key);
        ctrl->self_key = NULL;
    }
}

static int fakecert_SetIssuer(FAKECERT_CTRL_S *ctrl, X509 *cert)
{
    return X509_set_issuer_name(cert, X509_get_subject_name(ctrl->ca_cert));
}

static int fakecert_SetPublicKey(FAKECERT_CTRL_S *ctrl, X509 *cert)
{
    return X509Cert_SetPublicKey(cert, ctrl->self_key);
}

static int fakecert_Sign(FAKECERT_CTRL_S *ctrl, X509 *cert)
{
    return X509Cert_Sign(cert, ctrl->ca_key);
}

int fakecert_AddExt(FAKECERT_CTRL_S *ctrl, X509 *cert)
{
    int loc;
    X509_EXTENSION *ext;
    ASN1_OCTET_STRING  *value;

    loc = X509_get_ext_by_NID(ctrl->ca_cert, NID_subject_key_identifier, -1);
    if (loc < 0) {
        return -1;
    }

    ext = X509_get_ext(ctrl->ca_cert, loc);
    value = X509_EXTENSION_get_data(ext);
    ext = X509_EXTENSION_create_by_NID(NULL, NID_authority_key_identifier, 0, value);
    X509_add_ext(cert, ext, -1);
    X509_EXTENSION_free(ext);

    return 0;
}

static int fakecert_is_reserved(int nid)
{
    static int saved_nids[] = {NID_basic_constraints, NID_key_usage, NID_ext_key_usage, NID_subject_alt_name}; 
    int i;
    
    for (i=0; i<sizeof(saved_nids)/sizeof(int); i++) {
        if (nid == saved_nids[i]) {
            return 1;
        }
    }

    return 0;
}

static void fakecert_DelExts(X509 *cert)
{
    int i, count;
    X509_EXTENSION *ext;
    int nid;

    count = X509_get_ext_count(cert);

    for (i= count -1; i>=0; i--) {
        ext = X509_get_ext(cert, i);
        nid = OBJ_obj2nid(X509_EXTENSION_get_object(ext));
        if (! (fakecert_is_reserved(nid))) {
            X509Cert_DelExt(cert, nid);
        }
    }
}

static int fakecert_InitCert(X509 * pstX509)
{
    INT iRet;
    ULONG ulSecsOneDay;
    ULONG ulTime;
    ASN1_INTEGER *pstSerialNumber;
    ASN1_TIME *pstTime;
    ASN1_TIME *pstTimeRet;   
    X509_NAME *pstName = NULL;
    X509_EXTENSION *pstExtention = NULL;
    
    
    iRet = X509_set_version(pstX509, 2);
    if (1 !=  iRet) {
        return -1;
    }

    
    pstSerialNumber = X509_get_serialNumber(pstX509);
    iRet = ASN1_INTEGER_set(pstSerialNumber,0);
    if (1 != iRet) {
        return -1;
    }

    
    pstTime = X509_get_notBefore(pstX509);
    pstTimeRet = X509_gmtime_adj(pstTime,(LONG)0);
    if (NULL == pstTimeRet) {
        return -1;
    }
    pstTime = X509_get_notAfter(pstX509);
    ulSecsOneDay = (60 * 60) * 24;
    ulTime = 7300 * ulSecsOneDay;

    
    pstTimeRet = X509_gmtime_adj(pstTime, (LONG)ulTime);
    if (NULL == pstTimeRet) {
        return -1;
    }

    
    pstName = X509_get_subject_name(pstX509);
    iRet = X509_NAME_add_entry_by_txt(pstName, "CN", MBSTRING_ASC, (void*)"test", -1, -1, 0);
    if (1 != iRet) {
        return -1;
    }
    
    
    pstExtention = X509V3_EXT_conf_nid(NULL, NULL, NID_basic_constraints, "CA:FALSE");
    if (NULL == pstExtention) {
        return -1;
    }

    
    iRet = X509_add_ext(pstX509, pstExtention, -1);

    
    X509_EXTENSION_free(pstExtention);

    if (1 != iRet) {
        return -1;
    }
    
    return 0;
}


int FakeCert_BuildByCert(FAKECERT_CTRL_S *ctrl, X509 *cert)
{
    int ret;

    ret = fakecert_SetIssuer(ctrl, cert);
    ret |= fakecert_SetPublicKey(ctrl, cert);
    fakecert_DelExts(cert);
    ret |= fakecert_Sign(ctrl, cert);

    if (ret != 0) {
        return -1;
    }

    return 0;
}

X509 * FakeCert_Build(FAKECERT_CTRL_S *ctrl)
{
    X509 *pstX509;
    int ret;

    
    pstX509 = X509_new();
    if (NULL == pstX509) {
        return NULL;
    }

    
    ret = fakecert_InitCert(pstX509);
    if (0 != ret) {
        X509_free(pstX509);
        return NULL;
    }

    FakeCert_BuildByCert(ctrl, pstX509);

    return pstX509;
}
