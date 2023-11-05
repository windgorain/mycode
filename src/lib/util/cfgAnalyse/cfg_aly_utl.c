/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-5-18
* Description: 
* History:     
******************************************************************************/

#define RETCODE_FILE_NUM RETCODE_FILE_NUM_CFGALY

#include "bs.h"

#include "utl/cfg_aly_utl.h"
#include "utl/txt_utl.h"



typedef struct
{
    UCHAR ucSplitChar;
    CHAR acBufReserved[CFG_ALY_MAX_LINE_LEN + 1];
    UINT ulBufReservedLen;
}_CFG_ALY_CTRL_S;

BS_STATUS CFG_ALY_Create(IN UCHAR ucSplitChar, OUT HANDLE *phHandle)
{
    _CFG_ALY_CTRL_S *pstCtrl;

    pstCtrl = MEM_Malloc(sizeof(_CFG_ALY_CTRL_S));
    if (NULL == pstCtrl)
    {
        RETURN(BS_NO_MEMORY);
    }

    Mem_Zero(pstCtrl, sizeof(_CFG_ALY_CTRL_S));
    pstCtrl->ucSplitChar = ucSplitChar;

    *phHandle = pstCtrl;

    return BS_OK;    
}

BS_STATUS CFG_ALY_Delete(IN HANDLE hHandle)
{
    if (hHandle == 0)
    {
        BS_WARNNING(("Null ptr!"));
        RETURN(BS_NULL_PARA);
    }

    MEM_Free(hHandle);

    return BS_OK;
}

BS_STATUS CFG_ALY_FindKeyValueByStream
(
    IN HANDLE hHandle,
    IN CHAR *pcBuf, 
    IN CHAR *pcKeyName, 
    OUT CHAR *pcValue,
    IN UINT ulMaxValueLen
)
{
    _CFG_ALY_CTRL_S *pstCtrl = (_CFG_ALY_CTRL_S *)hHandle;
    UINT ulLineLen;
    BOOL_T bIsFindLineEnd;
    CHAR *pcLineNext;
    CHAR *pcBufTmp = pcBuf;
    CHAR *pcSplitChar;

    if (pstCtrl == NULL)
    {
        BS_WARNNING(("Null ptr!"));
        RETURN(BS_NULL_PARA);
    }

    while (pcBufTmp)
    {
        TXT_GetLine(pcBufTmp, &ulLineLen, &bIsFindLineEnd, &pcLineNext);
        if (ulLineLen != 0)
        {
            if (ulLineLen + pstCtrl->ulBufReservedLen > CFG_ALY_MAX_LINE_LEN)
            {
                BS_WARNNING(("Too long line!"));
                RETURN(BS_NOT_SUPPORT);
            }

            MEM_Copy(pstCtrl->acBufReserved, pcBufTmp, ulLineLen);
            pstCtrl->ulBufReservedLen += ulLineLen;
            pstCtrl->acBufReserved[pstCtrl->ulBufReservedLen] = '\0';
        }

        if (bIsFindLineEnd == FALSE)
        {
            RETURN(BS_NOT_FOUND);
        }
        else
        {
            if (pstCtrl->ucSplitChar != ' ')
            {
                TXT_DelSubStr(pstCtrl->acBufReserved, " ", pstCtrl->acBufReserved, sizeof(pstCtrl->acBufReserved));
            }
            if (pstCtrl->ucSplitChar != '\t')
            {
                TXT_DelSubStr(pstCtrl->acBufReserved, "\t", pstCtrl->acBufReserved, sizeof(pstCtrl->acBufReserved));                
            }
        
            if (strncmp(pstCtrl->acBufReserved, pcKeyName, strlen(pcKeyName)) == 0)
            {
                pcSplitChar = strchr(pstCtrl->acBufReserved, pstCtrl->ucSplitChar);
                if (pcSplitChar != NULL)
                {
                    if (strlen(pcSplitChar + 1) > ulMaxValueLen)
                    {
                        pstCtrl->ulBufReservedLen = 0;
                        pstCtrl->acBufReserved[0] = '\0';
                        BS_WARNNING(("Too long line!"));
                        RETURN(BS_NOT_SUPPORT);
                    }
                    
                    TXT_StrCpy(pcValue, pcSplitChar + 1);
                    pstCtrl->ulBufReservedLen = 0;
                    pstCtrl->acBufReserved[0] = '\0';
                    return BS_OK;
                }
            }
        }

        pstCtrl->ulBufReservedLen = 0;
        pstCtrl->acBufReserved[0] = '\0';
        
        pcBufTmp = pcLineNext;
    }

    RETURN(BS_NOT_FOUND);
}

