/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-10-26
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/file_utl.h"
#include "utl/getopt2_utl.h"

static UINT g_uiStartAddress = 0;
static UINT g_uiStopAddress = 0;
static CHAR *g_pcFromFile = NULL;
static CHAR *g_pcToFile = NULL;

static GETOPT2_NODE_S g_astCopySubOpts[] =
{
    {'o', 0, "start-address", GETOPT2_V_U32, &g_uiStartAddress, "start address", 0},
    {'o', 0, "stop-address", GETOPT2_V_U32, &g_uiStopAddress, "stop address"0},
    {'p', 0, NULL, GETOPT2_V_STRING, &g_pcFromFile, NULL, 0},
    {'p', 0, NULL, GETOPT2_V_STRING, &g_pcToFile, NULL, 0},
    {0}
};

static VOID copysub_help()
{
    printf(
        "Usage: copysub [option] from-file to-file\r\n"
        "Option:\r\n"
        "  --start-address\r\n"
        "  --stop-address\r\n"
        "\r\n"
        );
}

/* 
    copysub [option] fromFile [toFile].
*/
VOID main(IN INT uiArgc, IN CHAR **ppcArgv)
{
    FILE *pstFrom, *pstTo = NULL;
    S64 filesize;
    UCHAR ucChar;
    UINT uiStart;

    if (uiArgc < 2)
    {
        copysub_help();
        return;
    }
    
    if (BS_OK != GETOPT2_Parse(uiArgc, ppcArgv, g_astCopySubOpts)) {
        copysub_help();
        return;
    }

    if (g_uiStartAddress != 0)
    {
        if (g_uiStartAddress > g_uiStopAddress)
        {
            return;
        }
    }

    filesize = FILE_GetSize(g_pcFromFile);
    if (filesize < 0) {
        return;
    }

    if (g_uiStartAddress >= filesize)
    {
        return;
    }

    pstFrom = FILE_Open(g_pcFromFile, FALSE, "rb");
    if (NULL == pstFrom)
    {
        return;
    }

    if (g_pcToFile != NULL)
    {
        pstTo = FILE_Open(g_pcToFile, TRUE, "wb+");
        if (NULL == pstTo)
        {
            fclose(pstFrom);
            return;
        }
    }

    fseek(pstFrom, g_uiStartAddress, SEEK_SET);

    uiStart = g_uiStartAddress;
    
    ucChar = (UCHAR)fgetc(pstFrom);

    while (!feof(pstFrom))
    {
        if ((g_uiStopAddress != 0) && (uiStart > g_uiStopAddress))
        {
            break;
        }
        
        if (pstTo)
        {
            fputc(ucChar, pstTo);
        }
        else
        {
            printf("%c", ucChar);
        }
        
        ucChar = (UCHAR)fgetc(pstFrom);
        uiStart ++;
    }

    fclose(pstFrom);
    if (pstTo)
    {
        fclose(pstTo);
    }
    
    return;
}


