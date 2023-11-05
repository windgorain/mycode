/*================================================================
*   Created by LiXingang: 2018.11.11
*   Description: 
*
================================================================*/
#ifndef _FAKECERT_DNSNAMES_H
#define _FAKECERT_DNSNAMES_H
#include "utl/pki_utl.h"
#ifdef __cplusplus
extern "C"
{
#endif

void fakecert_dnsnames_init();
char * fakecert_dnsnames_add(X509 *cert, char *certname);
char * fakecert_dnsnames_find(char *hostname);

#ifdef __cplusplus
}
#endif
#endif 
