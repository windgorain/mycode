/*================================================================
*   Created by LiXingang: 2018.11.11
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/pki_utl.h"
#include "utl/txt_utl.h"
#include "utl/file_utl.h"
#include "utl/dnsname_trie.h"
#include "fakecert_dnsnames.h"

static TRIE_HANDLE g_fakecert_dnsnames_trie;

static BS_WALK_RET_E _fakecert_dnsnames_foreach_cb(ASN1_STRING *alt_name, void *user_data)
{
    DnsNameTrie_Insert(g_fakecert_dnsnames_trie, (void*)alt_name->data, strlen((void*)alt_name->data), user_data);
    return BS_WALK_CONTINUE;
}

static void _fakecert_dnsname_process_file(IN char *filename)
{
    char buf[512];
    void *cert;

    snprintf(buf, sizeof(buf), "trusted/%s", filename);

    cert = PKI_LoadCertByPemFile(buf, NULL, NULL);
    if (NULL == cert) {
        return;
    }

    fakecert_dnsnames_add(cert, filename);

    PKI_FreeX509Cert(cert);

    return;
}

/* 遍历可信证书库加载所有证书的dnsname */
static void _fakecert_dnsname_cert_dir_init()
{
    char *filename;

    FILE_SCAN_START("./trusted", filename) {
        _fakecert_dnsname_process_file(filename);
    }FILE_SCAN_END();
}

void fakecert_dnsnames_init()
{
    g_fakecert_dnsnames_trie = Trie_Create(TRIE_TYPE_4BITS);
    _fakecert_dnsname_cert_dir_init();
}

/* 返回存储证书名指针 */
char * fakecert_dnsnames_add(X509 *cert, char *certname)
{
    char *certname_new = strdup(certname);

    if (certname_new == NULL) {
        return NULL;
    }

    X509Cert_AltNameForEach(cert, _fakecert_dnsnames_foreach_cb, certname_new);

    return certname_new;
}

char * fakecert_dnsnames_find(char *hostname)
{
    return DnsNameTrie_Match(&g_fakecert_dnsnames_trie, hostname, strlen(hostname), TRIE_MATCH_WILDCARD);
}
