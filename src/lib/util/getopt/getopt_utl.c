/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-10-26
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/getopt_utl.h"

typedef struct
{
    UINT uiAgrc;
    CHAR **ppcArgv;
    CHAR *pcOpt;

    UINT uiOffset;
}_GETOPT_CTRL_S;

GETOPT_HANDLE GETOPT_Create
(
    IN UINT uiAgrc,
    IN CHAR **ppcArgv,
    IN CHAR *pcOpt
)
{
    _GETOPT_CTRL_S *pstCtrl;

    if ((uiAgrc <= 0) ||  (ppcArgv == NULL) || (pcOpt == NULL))
    {
        return NULL;
    }

    pstCtrl = MEM_ZMalloc(sizeof(_GETOPT_CTRL_S));
    if (NULL == pstCtrl)
    {
        return NULL;
    }

    pstCtrl->uiAgrc = uiAgrc;
    pstCtrl->ppcArgv = ppcArgv;
    pstCtrl->pcOpt = pcOpt;

    return pstCtrl;
}

VOID GETOPT_Destory(IN GETOPT_HANDLE hGetOptHandle)
{
    _GETOPT_CTRL_S *pstCtrl = hGetOptHandle;

    MEM_Free(pstCtrl);
}

INT GETOPT_GetOpt
(
    IN GETOPT_HANDLE hGetOptHandle,
    OUT CHAR **ppcValue
)
{
    UINT i;
    _GETOPT_CTRL_S *pstCtrl = hGetOptHandle;
    CHAR *pcFind;

    *ppcValue = "";

    for (i=pstCtrl->uiOffset; i<pstCtrl->uiAgrc; i++)
    {
        
        if (pstCtrl->ppcArgv[i][0] != '-')
        {
            continue;
        }

        if (pstCtrl->ppcArgv[i][1] == '-')
        {
            
        }

        pcFind = strchr(pstCtrl->pcOpt, pstCtrl->ppcArgv[i][1]);
        if (pcFind[1] == ':')
        {
            if (pstCtrl->uiAgrc >= i + 1)
            {
                return -1;
            }

            *ppcValue = pstCtrl->ppcArgv[i + 1];
        }

        return pstCtrl->ppcArgv[i][1];
    }

    return -1;
}



