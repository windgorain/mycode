/*================================================================
*   Created：2018.06.26
*   Description：
*
================================================================*/
#ifndef _FAKE_CERT_H
#define _FAKE_CERT_H

#include <openssl/x509.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/ocsp.h>
#include <openssl/bn.h>

#ifdef __cplusplus
extern "C"
{
#endif

int fakecert_init();
int fakecert_build_by_dnsname(unsigned int ip/*netorder*/, unsigned short port, char *host_name);
char * fakecert_build_by_cert(void *cert, char *host_name);
int fakecert_create_by_hostname(char *host_name);
void * fakecert_get_trusted_cert(void *realcert, char *domain_name);

#ifdef __cplusplus
}
#endif
#endif //FAKE_CERT_H_
