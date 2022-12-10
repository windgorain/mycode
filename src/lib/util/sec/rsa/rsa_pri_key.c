/*================================================================
*   Created by LiXingang: 2018.12.11
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/rsa_utl.h"
#include <openssl/decoder.h>

static char * g_private_key =
"-----BEGIN RSA PRIVATE KEY-----\n"
"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\n"
"-----END RSA PRIVATE KEY-----";

EVP_PKEY * RSA_DftPrivateKey()
{
    OSSL_DECODER_CTX *dctx;
    EVP_PKEY *pkey = NULL;
    const char *format = "PEM";   /* NULL for any format */
    const char *structure = NULL; /* any structure */
    const char *keytype = "RSA";  /* NULL for any key */
    const char *pass = NULL;
    BIO* bio = NULL;

    if ((bio = BIO_new_mem_buf(g_private_key, -1)) == NULL) {     
        return NULL;
    }

    dctx = OSSL_DECODER_CTX_new_for_pkey(&pkey, format, structure, keytype, OSSL_KEYMGMT_SELECT_KEYPAIR, NULL, NULL);
    if (dctx) {
        if (pass) {
            OSSL_DECODER_CTX_set_passphrase(dctx, (void*)pass, strlen(pass));
        }
        OSSL_DECODER_from_bio(dctx, bio);
        OSSL_DECODER_CTX_free(dctx);
    }

    BIO_free(bio);

    return pkey;
}
