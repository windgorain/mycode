/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2015-12-21
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/file_utl.h"
#include "utl/drp_utl.h"
#include "utl/txt_utl.h"
#include "utl/kv_utl.h"
#include "utl/getopt2_utl.h"

typedef struct
{
    LSTR_S stPath;
    LSTR_S stFileName;
    LSTR_S stFileNameWithOutExt;
    LSTR_S stExtName;
}FILESCAN_FILEINFO_S;

static CHAR *g_pcFileScanPath = NULL;
static CHAR *g_pcFileScanModuleFile = NULL;
static CHAR *g_pcFileScanPattern = NULL;
static CHAR *g_pcFileScanKeyList = NULL;
static FILE_MEM_S *g_pstFileScanMouldMemMap;  /* 模板内容 */
static DRP_HANDLE g_hFileScanDrp;

static GETOPT2_NODE_S g_astFileScanOpts[]    =
{
    { 'o', 0, "key-list", 's', &g_pcFileScanKeyList, "key list", 0 },
    { 'p', 0, NULL, 's', &g_pcFileScanPath, NULL, 0 },
    { 'p', 0, NULL, 's', &g_pcFileScanModuleFile, NULL, 0 },
    { 'p', 0, NULL, 's', &g_pcFileScanPattern,  NULL, 0 },
    {0}
};

static VOID filescan_ScanFileOutput
(
    IN CHAR *pcCurrentScanDir, /* 文件所在的相对于UserScanDir的相对目录 */
    IN CHAR *pcFileName,
    IN VOID *pUserData
)
{
    FILESCAN_FILEINFO_S stFileInfo;
    DRP_FILE hDrpFile;
    CHAR szData[512];
    INT iReadLen;

    stFileInfo.stPath.pcData = pcCurrentScanDir;
    stFileInfo.stPath.uiLen = strlen(pcCurrentScanDir);
    stFileInfo.stFileName.pcData = pcFileName;
    stFileInfo.stFileName.uiLen = strlen(pcFileName);
    FILE_GetFileNameWithOutExtFromPath(pcFileName, &stFileInfo.stFileNameWithOutExt);
    stFileInfo.stExtName.uiLen = 0;
    stFileInfo.stExtName.pcData = FILE_GetExternNameFromPath(pcFileName, stFileInfo.stFileName.uiLen);
    if (NULL != stFileInfo.stExtName.pcData)
    {
        stFileInfo.stExtName.uiLen = strlen(stFileInfo.stExtName.pcData);
    }

    hDrpFile = DRP_FileOpen(g_hFileScanDrp, g_pcFileScanModuleFile, &stFileInfo);
    if (hDrpFile == NULL)
    {
        printf("Can't open %s\r\n", g_pcFileScanModuleFile);
        return;
    }

    while (1)
    {
        if (DRP_FileEOF(hDrpFile))
        {
            break;
        }
        iReadLen = DRP_FileRead(hDrpFile, szData, sizeof(szData) - 1);
        szData[iReadLen] = '\0';

        printf("%s", szData);
    }

    DRP_FileClose(hDrpFile);

    return;
}

static BS_STATUS filescan_ProcessFileName
(
    IN DRP_HANDLE hDrp,
    IN LSTR_S *pstKey,
    IN VOID *pDrpCtx,
    IN HANDLE hUserHandle,
    IN HANDLE hUserHandle2
)
{
    FILESCAN_FILEINFO_S *pstFileInfo = hUserHandle;

    return DRP_CtxOutput(pDrpCtx, pstFileInfo->stFileName.pcData, pstFileInfo->stFileName.uiLen);
}

static BS_STATUS filescan_ProcessFileNameWithOutExt
(
    IN DRP_HANDLE hDrp,
    IN LSTR_S *pstKey,
    IN VOID *pDrpCtx,
    IN HANDLE hUserHandle,
    IN HANDLE hUserHandle2
)
{
    FILESCAN_FILEINFO_S *pstFileInfo = hUserHandle;

    return DRP_CtxOutput(pDrpCtx, pstFileInfo->stFileNameWithOutExt.pcData, pstFileInfo->stFileNameWithOutExt.uiLen);
}

static BS_STATUS filescan_ProcessPath
(
    IN DRP_HANDLE hDrp,
    IN LSTR_S *pstKey,
    IN VOID *pDrpCtx,
    IN HANDLE hUserHandle,
    IN HANDLE hUserHandle2
)
{
    FILESCAN_FILEINFO_S *pstFileInfo = hUserHandle;

    return DRP_CtxOutput(pDrpCtx, pstFileInfo->stPath.pcData, pstFileInfo->stPath.uiLen);
}

static BS_STATUS filescan_ProcessExtName
(
    IN DRP_HANDLE hDrp,
    IN LSTR_S *pstKey,
    IN VOID *pDrpCtx,
    IN HANDLE hUserHandle,
    IN HANDLE hUserHandle2
)
{
    FILESCAN_FILEINFO_S *pstFileInfo = hUserHandle;

    return DRP_CtxOutput(pDrpCtx, pstFileInfo->stExtName.pcData, pstFileInfo->stExtName.uiLen);
}

static BS_STATUS filescan_ProcessKey
(
    IN DRP_HANDLE hDrp,
    IN LSTR_S *pstKey,
    IN VOID *pDrpCtx,
    IN HANDLE hUserHandle,
    IN HANDLE hUserHandle2
)
{
    CHAR *pcValue = hUserHandle2;

    if (TXT_IS_EMPTY(pcValue))
    {
        return BS_OK;
    }

    return DRP_CtxOutput(pDrpCtx, pcValue, strlen(pcValue));
}

static BS_WALK_RET_E filescan_WalkKeyList(IN CHAR *pcKey, IN CHAR *pcValue, IN HANDLE hUserHandle)
{
    DRP_Set(g_hFileScanDrp, pcKey, filescan_ProcessKey, pcValue);

    return BS_WALK_CONTINUE;
}

static VOID filescan_help()
{
    printf(
        "Usage: filescan [option] path module-file [pattern] \r\n"
        "Option:\r\n"
        "  --key-list\r\n"
        "\r\n"
        );
}

int main(int argc, char* argv[])
{
    KV_HANDLE hKv;
    LSTR_S stLstr;
    
    if (BS_OK != GETOPT2_Parse(argc, argv, g_astFileScanOpts)) {
        filescan_help();
        return;
    }

    if ((NULL == g_pcFileScanPath) || (NULL == g_pcFileScanModuleFile))
    {
        filescan_help();
        return;
    }

    g_hFileScanDrp = DRP_Create("%", "%");
    if (NULL == g_hFileScanDrp)
    {
        return -1;
    }

    DRP_Set(g_hFileScanDrp, "FileName", filescan_ProcessFileName, NULL);
    DRP_Set(g_hFileScanDrp, "FileNameWithOutExt", filescan_ProcessFileNameWithOutExt, NULL);
    DRP_Set(g_hFileScanDrp, "Path", filescan_ProcessPath, NULL);
    DRP_Set(g_hFileScanDrp, "ExtName", filescan_ProcessFileName, NULL);

    if (NULL != g_pcFileScanKeyList)
    {
        hKv = KV_Create(0);
        if (hKv == NULL)
        {
            return -1;
        }

        stLstr.pcData = g_pcFileScanKeyList;
        stLstr.uiLen  = strlen(g_pcFileScanKeyList);

        KV_Parse(hKv, &stLstr, ',', '=');

        KV_Walk(hKv, filescan_WalkKeyList, NULL);
    }

    FILE_ScanFile(g_pcFileScanPath, g_pcFileScanPattern, filescan_ScanFileOutput, NULL);

    return 0;
}


