#include "bs.h"

#include "utl/file_utl.h"
#include "utl/txt_utl.h"
#include "utl/pki_utl.h"
#include "openssl/ssl.h"
#include "openssl/pkcs12.h"
#include "openssl/x509v3.h"

int X509Cert_SetIssuer(X509 *cert, X509_NAME *issuer)
{
    X509_set_issuer_name(cert, issuer);

    return 0;
}

/* 更改证书的public key */
int X509Cert_SetPublicKey(X509 *cert, void *pkey)
{
    if (1 != X509_set_pubkey(cert, pkey)) {
        return -1;
    }

    return 0;
}

int X509Cert_Sign(X509 *cert, void *ca_key)
{
    EVP_MD *evp_md;

    evp_md = (void*)EVP_sha256();

    if (0 == X509_sign(cert, ca_key, evp_md)) {
        return -1;
    }

    return 0;
}

char * X509Cert_GetSubjectName(X509 *pstCert, char *info, int info_size)
{
    char *pszOriginalSubj;
    int nSize=0;

    // 获取真实证书的持有者信息
    pszOriginalSubj = X509_NAME_oneline(X509_get_subject_name(pstCert),0,0);
    if (pszOriginalSubj==NULL)  {
        return NULL;
    }

    nSize = strlen(pszOriginalSubj);
    if (nSize<=0)  {
        OPENSSL_free(pszOriginalSubj);
        return NULL;
    }

    if (nSize >= info_size) {
        OPENSSL_free(pszOriginalSubj);
        return NULL;
    }

    strcpy(info, pszOriginalSubj);

    OPENSSL_free(pszOriginalSubj);

    return info;
}

char * fakecert_convert_info(char *old_info, char *new_info, int new_info_size)
{
    char *c;
    char *dst;
    char *dst_end = new_info + new_info_size - 1;

    c = old_info;
    dst = new_info;

    while (*c != '\0') {
        if (dst >= dst_end) {
            return NULL;
        }

        if ((*c == ' ') || (*c == '(') || (*c == ')')) {
            *dst = '\\';
            dst++;
            if (dst >= dst_end) {
                return NULL;
            }
        }
        *dst = *c;
        dst++;
        c++;
    }

    *dst = '\0';

    return new_info;
}

char * X509Cert_Str2Translate(char *in, char *out, int out_size)
{
    return TXT_Str2Translate(in, " ()", out, out_size);
}

void X509Cert_DelExt(X509 *cert, int nid)
{
    int loc;
    X509_EXTENSION *ext;

    while (1) {
        loc = X509_get_ext_by_NID(cert, nid, -1);
        if (loc >= 0) {
            ext = X509_delete_ext(cert, loc);
            X509_EXTENSION_free(ext);
        } else {
            break;
        }
    }
}

void X509Cert_AltNameForEach(X509 *cert, pf_X509Cert_AltNameForEachCB func, void *user_data)
{
    GENERAL_NAMES *gens;
    int i;

    gens = X509_get_ext_d2i(cert, NID_subject_alt_name, NULL, NULL);
    if (gens) {
        for (i = 0; i < sk_GENERAL_NAME_num(gens); i++) {
            GENERAL_NAME *gen;
            ASN1_STRING *cstr;

            gen = sk_GENERAL_NAME_value(gens, i);
            if (gen->type != GEN_DNS)
                continue;

            cstr = gen->d.dNSName;

            if (BS_WALK_CONTINUE != func(cstr, user_data)) {
                break;
            }
        }
        GENERAL_NAMES_free(gens);
    }
}
