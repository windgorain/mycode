#include "bs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/x509_vfy.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include "utl/cert_util.h"

extern BIO *bio_err;

static int verify_cb(int ok, X509_STORE_CTX *ctx);

static int check_cert(X509_STORE *ctx, X509 *cert,
        STACK_OF(X509) *uchain, STACK_OF(X509) *tchain,
        STACK_OF(X509_CRL) *crls, int show_chain);

X509_STORE *setup_verify(const char *CAfile, const char *CApath, int noCAfile, int noCApath)
{
    X509_STORE *store = X509_STORE_new();
    X509_LOOKUP *lookup;

    if (store == NULL)
        goto end;

    if (CAfile != NULL || !noCAfile) {
        lookup = X509_STORE_add_lookup(store, X509_LOOKUP_file());
        if (lookup == NULL)
            goto end;
        if (CAfile) {
            if (!X509_LOOKUP_load_file(lookup, CAfile, X509_FILETYPE_PEM)) {
                BIO_printf(bio_err, "Error loading file %s\n", CAfile);
                goto end;
            }
        } else {
            X509_LOOKUP_load_file(lookup, NULL, X509_FILETYPE_DEFAULT);
        }
    }

    if (CApath != NULL || !noCApath) {
        lookup = X509_STORE_add_lookup(store, X509_LOOKUP_hash_dir());
        if (lookup == NULL)
            goto end;
        if (CApath) {
            if (!X509_LOOKUP_add_dir(lookup, CApath, X509_FILETYPE_PEM)) {
                BIO_printf(bio_err, "Error loading directory %s\n", CApath);
                goto end;
            }
        } else {
            X509_LOOKUP_add_dir(lookup, NULL, X509_FILETYPE_DEFAULT);
        }
    }

    ERR_clear_error();
    return store;
end:
    X509_STORE_free(store);
    return NULL;
}

int cert_verify_by_file(const char *CApath, const char *CAfile, int noCApath, int noCAfile, const char *file)
{
    STACK_OF(X509_CRL) *crls = NULL;
    X509_STORE *store = NULL;
    int ret = 0;
    int show_chain = 1;
    X509 *vcert = NULL;

    if ((store = setup_verify(CAfile, CApath, noCAfile, noCApath)) == NULL)
        goto end;
   
    X509_STORE_set_verify_cb(store, verify_cb);

    if (file) {
        vcert = load_cert(file, FORMAT_PEM, "certificate file");
    } 

    if (!vcert) {
        goto end;
    }

    if (check_cert(store, vcert, NULL, NULL, crls, show_chain) != 1) {
        ret = -1;
    }

end:
    X509_STORE_free(store);

    return ret;
}

#if 0

int cert_verify_by_X509(STACK_OF(X509) *trusted, STACK_OF(X509) *untrusted, X509 *cert) 
{
    int show_chain = 1;

    X509_STORE *store = NULL;
    store=X509_STORE_new();
    X509_STORE_set_verify_cb(store, verify_cb);

    if (check_cert(store, cert, untrusted, trusted, NULL, show_chain) != 1) {
        goto end;
    }

end:
    X509_STORE_free(store);
}
#endif
static int check_cert(X509_STORE *ctx, X509 *verify_cert,
        STACK_OF(X509) *uchain, STACK_OF(X509) *tchain,
        STACK_OF(X509_CRL) *crls, int show_chain)
{
    int i = 0, ret = 0;
    X509_STORE_CTX *csc;
    STACK_OF(X509) *chain = NULL;

    csc = X509_STORE_CTX_new();
    if (csc == NULL) {
        printf("X.509 store context allocation failed\n");
        goto end;
    }

    X509_STORE_set_flags(ctx, 0);
    if (!X509_STORE_CTX_init(csc, ctx, verify_cert, uchain)) {
        X509_STORE_CTX_free(csc);
        printf("x.509 store context initialization failed\n");
        goto end;
    }
    if (tchain != NULL)
        X509_STORE_CTX_trusted_stack(csc, tchain);
        
    if (crls != NULL)
        X509_STORE_CTX_set0_crls(csc, crls);

    i = X509_verify_cert(csc);
    if (i > 0 && X509_STORE_CTX_get_error(csc) == X509_V_OK) {
        ret = 1;
        if (show_chain) {
            printf("OK\n");
            int j;
            chain = X509_STORE_CTX_get1_chain(csc);
            printf("Chain:\n");
            for (j = 0; j < sk_X509_num(chain); j++) {
                X509 *cert = sk_X509_value(chain, j);
                printf("depth=%d: ", j);
                X509_NAME_print_ex_fp(stdout,
                           X509_get_subject_name(cert),
                           0, XN_FLAG_ONELINE);
                printf("\n");
            }
            sk_X509_pop_free(chain, X509_free);
        }
    } else {
        printf("verification failed %d\n", X509_STORE_CTX_get_error(csc));
    }
    X509_STORE_CTX_free(csc);

end:
    if (i <= 0)
        ERR_print_errors(bio_err);
    X509_free(verify_cert);

    return ret;
}

static int verify_cb(int ok, X509_STORE_CTX *ctx)
{
    int cert_error = X509_STORE_CTX_get_error(ctx);
    X509 *current_cert = X509_STORE_CTX_get_current_cert(ctx);

    if (!ok) {
        if (current_cert != NULL) {
            X509_NAME_print_ex(bio_err,
                    X509_get_subject_name(current_cert),
                    0, XN_FLAG_ONELINE);
            BIO_printf(bio_err, "\n");
        }
#if 0
        BIO_printf(bio_err, "%serror %d at %d depth lookup: %s\n",
                X509_STORE_CTX_get0_parent_ctx(ctx) ? "[CRL path] " : "",
                cert_error,
                X509_STORE_CTX_get_error_depth(ctx),
                X509_verify_cert_error_string(cert_error));
#endif
        switch (cert_error) {
            case X509_V_ERR_CERT_HAS_EXPIRED:
            case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
            case X509_V_ERR_INVALID_CA:
            case X509_V_ERR_INVALID_NON_CA:
            case X509_V_ERR_PATH_LENGTH_EXCEEDED:
            case X509_V_ERR_INVALID_PURPOSE:
            case X509_V_ERR_CRL_HAS_EXPIRED:
            case X509_V_ERR_CRL_NOT_YET_VALID:
            case X509_V_ERR_UNHANDLED_CRITICAL_EXTENSION:
                ok = 1;
        }

        return ok;
    }

    ERR_clear_error();
    return ok;
}
