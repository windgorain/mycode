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

void X509Name_FreeAllEntry(X509_NAME *name)
{
    int count, i;
    X509_NAME_ENTRY *ne;

    count = X509_NAME_entry_count(name);
    for (i=count-1; i>=0; i--) {
        ne = X509_NAME_get_entry(name, i);
        X509_NAME_delete_entry(name, i);
        X509_NAME_ENTRY_free(ne);
    }
}

int X509Name_AddEntry(X509_NAME *name, char *field, char *value)
{
    if (1 != X509_NAME_add_entry_by_txt(name, field, MBSTRING_ASC, (const unsigned char*)value, -1, -1, 0)) {
        return -1;
    }

    return 0;
}
