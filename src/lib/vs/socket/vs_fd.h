/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-2-25
* Description: 
* History:     
******************************************************************************/

#ifndef __VS_FD_H_
#define __VS_FD_H_

#ifdef __cplusplus
    extern "C" {
#endif 


#define O_NONBLOCK      00004  

typedef struct
{
    VOID *pstSocket;
    UINT uiFlag;
}VS_FD_S;

#ifdef __cplusplus
    }
#endif 

#endif 


