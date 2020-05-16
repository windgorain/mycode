#include "bs.h"

#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/pkcs12.h>
#include <openssl/err.h>
#include <errno.h>
#include <stdio.h>
#include "utl/cert_util.h"

extern BIO* bio_err;

static const char *modestr(char mode, int format);

static int istext(int format)
{
    return (format & B_FORMAT_TEXT) == B_FORMAT_TEXT;
}

BIO *dup_bio_in(int format)
{
    return BIO_new_fp(stdin,
            BIO_NOCLOSE | (istext(format) ? BIO_FP_TEXT : 0));
}

BIO *dup_bio_out(int format)
{
    BIO *b = BIO_new_fp(stdout,
            BIO_NOCLOSE | (istext(format) ? BIO_FP_TEXT : 0));

    return b;
}

BIO *dup_bio_err(int format)
{
    BIO *b = BIO_new_fp(stderr,
            BIO_NOCLOSE | (istext(format) ? BIO_FP_TEXT : 0));
    return b;
}

static BIO *bio_open_default_(const char *filename, char mode, int format,
        int quiet)
{
    BIO *ret;

    if (filename == NULL || strcmp(filename, "-") == 0) {
        ret = mode == 'r' ? dup_bio_in(format) : dup_bio_out(format);
        if (quiet) {
            ERR_clear_error();
            return ret;
        }
        if (ret != NULL)
            return ret;
        BIO_printf(bio_err,
                "Can't open %s, %s\n",
                mode == 'r' ? "stdin" : "stdout", strerror(errno));
    } else {
        ret = BIO_new_file(filename, modestr(mode, format));
        if (quiet) {
            ERR_clear_error();
            return ret;
        }

        if (ret != NULL)
            return ret;

        BIO_printf(bio_err, "Can't open %s for \" %c \", %s\n",
                filename, mode, strerror(errno));
    }

    ERR_print_errors(bio_err);

    return NULL;
}

BIO *bio_open_default(const char *filename, char mode, int format)
{
    return bio_open_default_(filename, mode, format, 0);
}

X509 *load_cert(const char *file, int format, const char *cert_descrip)
{
    X509 *x = NULL;
    BIO *cert;

    if (file == NULL) {
        setbuf(stdin, NULL);
        cert = dup_bio_in(format);
    } else {
        cert = bio_open_default(file, 'r', format);
    }
    if (cert == NULL)
        goto end;

    if (format == FORMAT_ASN1) {
        x = d2i_X509_bio(cert, NULL);
    } else if (format == FORMAT_PEM) {
        x = PEM_read_bio_X509(cert, NULL, 0, NULL);
    } else {
        BIO_printf(bio_err, "bad input format specified for %s\n", cert_descrip);
        goto end;
    }

end:
    if (x == NULL) {
        BIO_printf(bio_err, "unable to load certificate\n");
        ERR_print_errors(bio_err);
    }
    BIO_free(cert);

    return x;
}

static const char *modestr(char mode, int format)
{
    OPENSSL_assert(mode == 'a' || mode == 'r' || mode == 'w');

    switch (mode) {
        case 'a':
            return istext(format) ? "a" : "ab";
        case 'r':
            return istext(format) ? "r" : "rb";
        case 'w':
            return istext(format) ? "w" : "wb";
    }
    /* The assert above should make sure we never reach this point */
    return NULL;
}

int load_certs_crls(const char *file, int format,
        const char *pass, const char *desc,
        STACK_OF(X509) **pcerts,
        STACK_OF(X509_CRL) **pcrls)
{
    int i;
    BIO *bio;
    STACK_OF(X509_INFO) *xis = NULL;
    X509_INFO *xi;
    int rv = 0;

    if (format != FORMAT_PEM) {
        BIO_printf(bio_err, "bad input format specified for %s\n", desc);
        return 0;
    }

    bio = bio_open_default(file, 'r', FORMAT_PEM);
    if (bio == NULL)
        return 0;

    xis = PEM_X509_INFO_read_bio(bio, NULL, NULL, NULL);
    BIO_free(bio);

    if (pcerts != NULL && *pcerts == NULL) {
        *pcerts = sk_X509_new_null();
        if (*pcerts == NULL)
            goto end;
    }

    if (pcrls != NULL && *pcrls == NULL) {
        *pcrls = sk_X509_CRL_new_null();
        if (*pcrls == NULL)
            goto end;
    }

    for (i = 0; i < sk_X509_INFO_num(xis); i++) {
        xi = sk_X509_INFO_value(xis, i);
        if (xi->x509 != NULL && pcerts != NULL) {
            if (!sk_X509_push(*pcerts, xi->x509))
                goto end;
            xi->x509 = NULL;
        }
        if (xi->crl != NULL && pcrls != NULL) {
            if (!sk_X509_CRL_push(*pcrls, xi->crl))
                goto end;
            xi->crl = NULL;
        }
    }
    
    printf("push cert num %d\n", sk_X509_num(*pcerts));
    if (pcerts != NULL && sk_X509_num(*pcerts) > 0)
        rv = 1;

    if (pcrls != NULL && sk_X509_CRL_num(*pcrls) > 0)
        rv = 1;

end:
    sk_X509_INFO_pop_free(xis, X509_INFO_free);

    if (rv == 0) {
        if (pcerts != NULL) {
            sk_X509_pop_free(*pcerts, X509_free);
            *pcerts = NULL;
        }
        if (pcrls != NULL) {
            sk_X509_CRL_pop_free(*pcrls, X509_CRL_free);
            *pcrls = NULL;
        }
        BIO_printf(bio_err, "unable to load %s\n",
                pcerts ? "certificates" : "CRLs");
        ERR_print_errors(bio_err);
    }
    return rv;
}
