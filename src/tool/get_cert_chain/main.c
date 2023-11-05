/*================================================================
*   Created：2018.06.26
*   Description：
*
================================================================*/
#include "bs.h"
#include <errno.h>
#include <getopt.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <fcntl.h>
#include <stdlib.h>
#include <openssl/x509.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/ocsp.h>
#include <openssl/bn.h>
#include "utl/ssl_utl.h"
#include "utl/pki_utl.h"

static void help(char *proc_name)
{
    printf("Usage: %s hostname\n", proc_name);
    return;
}

static unsigned int get_ip_by_name(char *name)
{
    struct hostent* remoteHost;
    remoteHost = gethostbyname(name);
    if (remoteHost != NULL)
    {
        return *((UINT*)remoteHost->h_addr_list[0]);
    }
    return 0;
}

static void save_cert(void *cert, char *name)
{
    PKI_SaveCertToPemFile(cert, name);
}

static void save_chain(void *ssl)
{
    STACK_OF(X509) *sk;
    char name[128];
    int i;
    X509 *cert;

    sk = SSL_get_peer_cert_chain(ssl);
    if (sk != NULL) {
        for (i = 0; i < sk_X509_num(sk); i++) {
            cert = sk_X509_value(sk, i);
            sprintf(name, "chain_%d.crt", i);
            save_cert(cert, name);
        }
    }
}

int get_cert_chain(unsigned int ip, unsigned short port, char *host_name)
{
    void *ssl;
    void *cert;
    int fd;
    int ret = -1;

    SSL_UTL_Init();

    ssl = SSL_UTL_BlockConnect(ip, port, host_name, 0);
    if (ssl == NULL) {
        printf("Cant' connect ssl server\r\n");
        return -1;
    }

    cert = SSL_get_peer_certificate(ssl);
    if (cert != NULL) {
        save_cert(cert, host_name);
        X509_free(cert);
    } else {
        printf("Cant' get cert \r\n");
    }

    save_chain(ssl);
    
    fd = SSL_get_fd(ssl);
    SSL_free(ssl);
    close(fd);

    return ret;
}

int main(int argc, char **argv)
{
    char *hostname;
    unsigned int ip;

    if (argc < 2) {
        help(argv[0]);
        return -1;
    }

    hostname = argv[1];

    ip = get_ip_by_name(hostname);

    get_cert_chain(ip, htons(443), hostname);

    return 0;
}
