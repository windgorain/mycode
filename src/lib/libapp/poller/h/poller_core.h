/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _POLLER_CORE_H
#define _POLLER_CORE_H
#include "utl/mypoll_utl.h"
#ifdef __cplusplus
extern "C"
{
#endif

#define POLLER_INS_MAX 16

#define POLLER_INS_NAME_LEN 15

typedef struct {
    char name[POLLER_INS_NAME_LEN + 1];
    THREAD_ID tid;
    MYPOLL_HANDLE mypoller;
    volatile UINT ref;
    DLL_HEAD_S ob_list;
}POLLER_INS_S;

void POLLER_INS_Init();
POLLER_INS_S * POLLER_INS_Add(char *ins_name);
int POLLER_INS_Del(POLLER_INS_S *ins);
int POLLER_INS_DelByName(char *name);
char * POLLER_INS_GetName(int id);
POLLER_INS_S * POLLER_INS_GetByName(char *name);
void POLLER_INS_Trigger(POLLER_INS_S *ins);

#ifdef __cplusplus
}
#endif
#endif 
