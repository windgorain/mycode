/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _PWATCHER_RECVER_H
#define _PWATCHER_RECVER_H

#include "utl/ubpf_utl.h"
#include "../h/pwatcher_def.h"

#ifdef __cplusplus
extern "C"
{
#endif

int PWatcherRecver_Init();
int PWatcherRecver_Run(char *recver_name, char *param);

#ifdef __cplusplus
}
#endif
#endif 
