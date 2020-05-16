/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-2-18
* Description: 
* History:     
******************************************************************************/

#ifndef __VNETS_WEB_H_
#define __VNETS_WEB_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

BS_STATUS VNETS_Web_Init();
BS_STATUS VNETS_WebKf_Init();
BS_STATUS VNETS_WebVldCode_Init();
BS_STATUS VNETS_WebUlm_Init();
BS_STATUS VNETS_CmdWeb_Save(IN HANDLE hFile);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETS_WEB_H_*/


