/*================================================================
*   Created：2018.06.25
*   Description：
*
================================================================*/
#include "bs.h"
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <stdlib.h>  

#include <openssl/x509.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/ocsp.h>
#include <openssl/bn.h>

#include "utl/pki_utl.h"
#include "utl/ssl_utl.h"
#include "utl/fake_cert.h"
#include "fakecert_lib.h"
#include "fakecert_dnsnames.h"

static FAKECERT_CTRL_S g_trusted_ctrl;
static FAKECERT_CTRL_S g_untrusted_ctrl;


static int _fakecert_is_cert_exist(char *fake_cert_name)
{
    char info[1024];
    int ret;

    ret = snprintf(info, sizeof(info), "%s.csr", fake_cert_name);
    if (ret >= sizeof(info)) {
        return 0;
    }

    if((access(fake_cert_name, F_OK)) < 0)  {   
        return 0;
    } 

    return 1;
}

/* 返回构造的证书名 */
char * fakecert_build_by_cert(void *cert, char *host_name)
{
    char cert_name[512];

    sprintf(cert_name, "real/%s.crt", host_name);
    PKI_SaveCertToPemFile(cert, cert_name);

    sprintf(cert_name, "trusted/%s.crt", host_name);
    FakeCert_BuildByCert(&g_trusted_ctrl, cert);
    PKI_SaveCertToPemFile(cert, cert_name);
    sprintf(cert_name, "cat trusted_chain.crt >> trusted/%s.crt", host_name);
    if (system(cert_name) < 0) {
        return NULL;
    }

    sprintf(cert_name, "untrusted/%s.crt", host_name);
    FakeCert_BuildByCert(&g_untrusted_ctrl, cert);
    PKI_SaveCertToPemFile(cert, cert_name);

    /*need add to tree after certificate created */
    sprintf(cert_name, "%s.crt", host_name);
    return fakecert_dnsnames_add(cert, cert_name);
}

int fakecert_init()
{
    SSL_UTL_Init();

    if (0 != FakeCert_init(&g_untrusted_ctrl, "untrusted_ca.crt", "untrusted_ca.key", "server.key", NULL, NULL)) {
        printf("Can't load untrusted ca environment.\r\n");
        return -1;
    }

    if (0 != FakeCert_init(&g_trusted_ctrl, "trusted_ca.crt", "trusted_ca.key", "server.key", NULL, "alilang")) {
        printf("Can't load untrusted root environment.\r\n");
        return -1;
    }

    return 0;
}

int fakecert_create_by_hostname(char *host_name)
{
    void *cert;
    char cert_name[512];

    sprintf(cert_name, "trusted/%s.crt", host_name);
    cert = FakeCert_Build(&g_trusted_ctrl);
    if (NULL == cert) {
        return -1;
    }
    PKI_SaveCertToPemFile(cert, cert_name);
    sprintf(cert_name, "cat trusted_chain.crt >> trusted/%s.crt", host_name);
    int ret  = system(cert_name);
    X509_free(cert);

    return ret;
}

/* 获取得到realcert，再根据realcert伪造证书 */
int fakecert_build_by_dnsname(unsigned int ip/*netorder*/, unsigned short port, char *host_name)
{
    void *ssl;
    void *cert;
    int fd;
    int ret = -1;

    ssl = SSL_UTL_BlockConnect(ip, port, host_name, 0);
    if (ssl == NULL) {
        printf("Cant' connect ssl server\r\n");
        return -1;
    }

    cert = SSL_get_peer_certificate(ssl);
    if (cert != NULL) {
        fakecert_build_by_cert(cert, host_name);
        X509_free(cert);
    } else {
        printf("Cant' get cert \r\n");
    }

    fd = SSL_get_fd(ssl);
    SSL_free(ssl);
    close(fd);

    return ret;
}

void * fakecert_get_trusted_cert(void *realcert, char *domain_name)
{
    char fakecert_name[1024];
    int ret;

    ret = snprintf(fakecert_name, sizeof(fakecert_name), "trusted/%s", domain_name);

    if (ret >= sizeof(fakecert_name)) {
        return NULL;
    }

    if (! _fakecert_is_cert_exist(fakecert_name)) {
        fakecert_build_by_cert(realcert, domain_name);

        if (! _fakecert_is_cert_exist(fakecert_name)) {
            return NULL;
        }
    }

    return PKI_LoadCertByPemFile(fakecert_name, NULL, NULL);
}

