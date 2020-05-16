#include "bs.h"

#include "utl/file_utl.h"
#include "utl/txt_utl.h"
#include "utl/pki_utl.h"
#include "openssl/ssl.h"
#include "openssl/pkcs12.h"
#include "openssl/x509v3.h"


void * PKI_LoadCertByPemFile(char *file, pem_password_cb *cb, void *u)
{
    BIO *in;
    X509 *x = NULL;

    in = BIO_new(BIO_s_file());
    if (in == NULL) {
        return NULL;
    }

    if (BIO_read_filename(in, file) > 0) {
        x = PEM_read_bio_X509(in, NULL, cb, u); 
    }

    BIO_free(in);

    return x;
}

void * PKI_LoadCertByAsn1File(char *file)
{
    BIO *in;
    X509 *x = NULL;

    in = BIO_new(BIO_s_file());
    if (in == NULL) {
        return NULL;
    }

    if (BIO_read_filename(in, file) > 0) {
        x = d2i_X509_bio(in, NULL);
    }

    BIO_free(in);

    return x;
}

void PKI_FreeX509Cert(void *cert)
{
    X509_free(cert);
}

void * PKI_LoadPrivateKeyByAsn1File(char *file)
{
    BIO *in;
    EVP_PKEY *pkey = NULL;

    in = BIO_new(BIO_s_file());
    if (in == NULL) {
        return NULL;
    }

    if (BIO_read_filename(in, file) > 0) {
        pkey = d2i_PrivateKey_bio(in, NULL);
    }

    BIO_free(in);

    return pkey;
}

void * PKI_LoadPrivateKeyByPemFile(char *file, pem_password_cb *cb, void *u)
{
    BIO *in;
    EVP_PKEY *pkey = NULL;

    in = BIO_new(BIO_s_file());
    if (in == NULL) {
        return NULL;
    }

    if (BIO_read_filename(in, file) > 0) {
        pkey = PEM_read_bio_PrivateKey(in, NULL, cb, u);
    }

    BIO_free(in);

    return pkey;
}

void *PKI_LoadPKCS8PrivateKeybyBuf(char *buf, pem_password_cb *cb, void*u)
{
    EVP_PKEY *evp_key = NULL;

    BIO* bio = BIO_new(BIO_s_mem());
    if (NULL == bio) return NULL;


    int len = BIO_write(bio, buf, strlen(buf));
    if (len <= 0) {
        BIO_free(bio);
    } else {
        PKCS8_PRIV_KEY_INFO *pkcs8_key = PEM_read_bio_PKCS8_PRIV_KEY_INFO(bio, &pkcs8_key, cb, u);
        if (pkcs8_key) {
            evp_key = EVP_PKCS82PKEY(pkcs8_key);
        }
    }
    BIO_free(bio);

    return evp_key;
}

void PKI_FreeKey(void *key)
{
    EVP_PKEY_free(key);
}

int PKI_SaveCertToPemFile(void *cert, char *file)
{
    BIO *Cout = NULL;

    Cout = BIO_new_file(file, "w");
    if (Cout == NULL) {
        return -1;
    }

    PEM_write_bio_X509(Cout, cert);
    BIO_free_all(Cout);

    return 0;
}
