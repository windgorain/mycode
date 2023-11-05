/*================================================================
*   Created by LiXingang: 2018.11.10
*   Description: 
*
================================================================*/
#ifndef _FAKECERT_SERVICE_H
#define _FAKECERT_SERVICE_H
#include "utl/cff_utl.h"
#ifdef __cplusplus
extern "C"
{
#endif

int fakecert_service_init();
int fakecert_service_conf_init(IN CFF_HANDLE hCff);
int fakecert_service_open_unix(char *unix_path);
int fakecert_service_open_tcp(unsigned short port);
void fakecert_service_run();

#ifdef __cplusplus
}
#endif
#endif 
