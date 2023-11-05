/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-7-20
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/telnet_server.h"
#include "utl/socket_utl.h"
#include "utl/exec_utl.h"
#include "utl/cmd_exp.h"
#include "utl/cff_utl.h"
#include "utl/mypoll_utl.h"



#define _TELSVR_DEL_CMD(pszData,ulLen)  \
    do{ \
        INT _i;    \
        for (_i=(ulLen)-1; _i>=0; _i--)  \
        {   \
            if ((((pszData)[_i] < 0x20) || ((pszData)[_i] > 0x80))  \
                && ((pszData)[_i] != '\n')  \
                && ((pszData)[_i] != '\r')  \
                && ((pszData)[_i] != '\t')  \
                && ((pszData)[_i] != '\b')) \
            {   \
                (ulLen)--;    \
            }   \
        }   \
    }while(0)


typedef enum
{
    _TELS_STATE_DATA = 0,
    _TELS_STATE_IAC,
    _TELS_STATE_WILL_WONT_DO_DONT,
    _TELS_STATE_SB,      
    _TELS_STATE_WAIT_SE, 

    _TELS_STATE_MAX
}_TELS_STATE_E;

static BS_STATUS _telsvr_ProcessData(IN TEL_CTRL_S *pstCtrl, IN UCHAR ucData)
{
    if (ucData == '\0') {
        return BS_OK;
    }

    if (CmdExp_Run(pstCtrl->hCmdRunner, ucData) == BS_STOP) {
        return BS_STOP;
    }

    return BS_OK;
}

static BS_STATUS _telsvr_Process(IN TEL_CTRL_S *pstCtrl, IN UCHAR ucData)
{
    if (pstCtrl->uiState == _TELS_STATE_DATA) {
        if (ucData == 255) {
            pstCtrl->uiState = _TELS_STATE_IAC;
            return BS_OK;
        }

        return _telsvr_ProcessData(pstCtrl, ucData);
    }

    if (pstCtrl->uiState == _TELS_STATE_IAC) {
        if (ucData == 255) {
            return _telsvr_ProcessData(pstCtrl, ucData);
        }

        if (ucData == 250) {
            pstCtrl->uiState = _TELS_STATE_SB;
            return BS_OK;
        }

        if ((ucData == 251) || (ucData == 252)
                || (ucData == 253) || (ucData == 254)) {
            pstCtrl->uiState = _TELS_STATE_WILL_WONT_DO_DONT;
            return BS_OK;
        }
    }

    if (pstCtrl->uiState == _TELS_STATE_WILL_WONT_DO_DONT) {
        pstCtrl->uiState = _TELS_STATE_DATA;
        return BS_OK;
    }

    if (pstCtrl->uiState == _TELS_STATE_SB) {
        if (ucData == 255) {
            pstCtrl->uiState = _TELS_STATE_WAIT_SE;
        }

        return BS_OK;
    }

    if (pstCtrl->uiState == _TELS_STATE_WAIT_SE) {
        pstCtrl->uiState = _TELS_STATE_DATA;
        return BS_OK;
    }

    return BS_OK;
}

void TELS_Init(TEL_CTRL_S *ctrl, int fd, HANDLE hCmdRunner)
{
    Mem_Zero(ctrl, sizeof(TEL_CTRL_S));
    ctrl->iSocketId = fd;
    ctrl->hCmdRunner = hCmdRunner;
}

void TELS_Hsk(int fd)
{
    CHAR *pcTmp = "\xff\xfb\03\xff\xfb\01";
    Socket_Write(fd, (UCHAR*)pcTmp, strlen(pcTmp), 0);
}

int TELS_Run(TEL_CTRL_S *ctrl, UCHAR *data, int data_len)
{
    int i;

    for (i=0; i<data_len; i++) {
        if (BS_STOP == _telsvr_Process(ctrl, data[i])) {
            return BS_STOP;
        }
    }

    return BS_OK;
}

