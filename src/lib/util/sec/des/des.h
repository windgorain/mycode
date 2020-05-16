/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-8-18
* Description: 
* History:     
******************************************************************************/

#ifndef __DES_H_
#define __DES_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define DES_KEY_SZ 	(sizeof(DES_cblock))
#define DES_SCHEDULE_SZ (sizeof(DES_key_schedule))

#define DES_ENCRYPT	1
#define DES_DECRYPT	0

#define DES_CBC_MODE	0
#define DES_PCBC_MODE	1

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__DES_H_*/


