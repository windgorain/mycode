/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2013-4-28
* Description: 
* History:     
******************************************************************************/

#ifndef __ARPCHEAT_MAIN_H_
#define __ARPCHEAT_MAIN_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

BS_STATUS ARPCheat_Main_Init();
VOID ARPCheat_Main_SetPcapIndex(IN UINT uiPcapIndex);
VOID ARPCheat_Main_SetCheatIP(IN UINT uiIP/* net order */);
VOID ARPCheat_Main_SetCheatMAC(IN MAC_ADDR_S *pstMAC);
BS_STATUS ARPCheat_Main_Start();
VOID ARPCheat_Main_Save(IN HANDLE hFile);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__ARPCHEAT_MAIN_H_*/


