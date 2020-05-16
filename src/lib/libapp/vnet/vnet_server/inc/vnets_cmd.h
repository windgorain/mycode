/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-4-22
* Description: 
* History:     
******************************************************************************/

#ifndef __VNETS_CMD_H_
#define __VNETS_CMD_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef BS_STATUS (*PF_VNETS_CMD_SAVE_FUNC)(IN HANDLE hFile);

BS_STATUS VNETS_CmdSave(IN HANDLE hFile);
BS_STATUS VNETS_CmdUdp_Save(IN HANDLE hFile);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETS_CMD_H_*/


