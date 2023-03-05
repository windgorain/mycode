/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      lixingang  Version: 1.0  Date: 2007-2-8
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "sys/stat.h"
#include "utl/mem_utl.h"
#include "utl/file_utl.h"
#include "utl/txt_utl.h"
#include "utl/cjson.h"
#include "utl/passwd_utl.h"
#include <stdbool.h>

/* funcs */
#ifdef IN_WINDOWS
static BOOL_T _OS_FILE_IsAbsolutePath(IN CHAR *pszPath)
{
    if (strchr(pszPath, ':') != NULL)
    {
        return TRUE;
    }

    return FALSE;
}
#endif

#ifdef IN_UNIXLIKE
static BOOL_T _OS_FILE_IsAbsolutePath(IN CHAR *pszPath)
{
    if (pszPath[0] == '/')
    {
        return TRUE;
    }

    return FALSE;
}
#endif

BOOL_T FILE_IsAbsolutePath(IN CHAR *pszPath)
{
    return _OS_FILE_IsAbsolutePath(pszPath);
}

char * FILE_ToAbsPath(char *base_dir, char *path, OUT char *dst, int dst_size)
{
    int old_path_len = strlen(path);
    int base_dir_len;
    int iRet;

    if (base_dir == NULL) {
        base_dir = "./";
    }

    if (old_path_len >= dst_size) {
        return NULL;
    }

    if (FILE_IsAbsolutePath(path)) {
        return path;
    } else {
        base_dir_len = strlen(base_dir);
        if ((base_dir[base_dir_len - 1] == '/') || (base_dir[base_dir_len - 1] == '\\')) {
            iRet = scnprintf(dst, dst_size, "%s%s", base_dir, path);
        } else {
            iRet = scnprintf(dst, dst_size, "%s/%s", base_dir, path);
        }
    }

    if ((iRet < 0) || (iRet >= dst_size)) {
        return NULL;
    }

    return dst;
}

char * FILE_Dup2AbsPath(IN char *base_dir, IN char *path)
{
    char filename[FILE_MAX_PATH_LEN + 1];
    char *new_filename;

    new_filename = FILE_ToAbsPath(base_dir, path, filename, sizeof(filename));
    if (NULL == new_filename) {
        return NULL;
    }

    return strdup(new_filename);
}

/* 从路径中获取文件名 */
CHAR * FILE_GetFileNameFromPath(IN CHAR *pszPath)
{
    UINT ulLen;

    BS_DBGASSERT(NULL != pszPath);

    ulLen = strlen(pszPath);

    while (ulLen > 0)
    {
        if ((pszPath[ulLen - 1] == '\\') || (pszPath[ulLen - 1] == '/'))
        {
            return pszPath + ulLen;
        }

        ulLen --;
    }

    return pszPath;
}

BS_STATUS FILE_GetFileNameWithOutExtFromPath(IN CHAR *pszPath, OUT LSTR_S *pstFileNameWithOutExt)
{
    CHAR *pcFileName;
    CHAR *pcExtName;

    BS_DBGASSERT(NULL != pszPath);

    pcFileName = FILE_GetFileNameFromPath(pszPath);
    if (NULL == pcFileName)
    {
        return BS_ERR;
    }

    pstFileNameWithOutExt->pcData = pcFileName;

    pcExtName = FILE_GetExternNameFromPath(pcFileName, strlen(pcFileName));
    if (NULL == pcExtName)
    {
        pstFileNameWithOutExt->uiLen = strlen(pcFileName);
    }
    else
    {
        pstFileNameWithOutExt->uiLen = ((pcExtName - pcFileName) - 1);
    }

    return BS_OK;
}

/* 从携带文件名的路径中,把路径取出来,路径以"/"结束 */
VOID FILE_GetPathFromFilePath(IN CHAR *pszFilePath, OUT CHAR *szPath)
{
    CHAR *pszFileName;
    
    BS_DBGASSERT(NULL != pszFilePath);

    szPath[0] = '\0';

    pszFileName = FILE_GetFileNameFromPath(pszFilePath);
    if (NULL == pszFileName)
    {
        BS_DBGASSERT(0);
        return;
    }

    if (pszFileName - pszFilePath > FILE_MAX_PATH_LEN)
    {
        BS_DBGASSERT(0);
        return;
    }

    MEM_Copy(szPath, pszFilePath, pszFileName - pszFilePath);
    szPath[pszFileName - pszFilePath] = '\0';

    return;
}

/* 不包含. */
CHAR * FILE_GetExternNameFromPath(IN CHAR *pszPath, IN UINT uiPathLen)
{
    UINT ulLen = uiPathLen;

    BS_DBGASSERT(NULL != pszPath);

    while (ulLen > 0)
    {
        if (pszPath[ulLen - 1] == '.')
        {
            return pszPath + ulLen;
        }

        ulLen --;
    }

    return NULL;
}

/* 抹去扩展名 */
CHAR * FILE_EarseExternName(IN CHAR *pcFilePath)
{
    CHAR *pcExtName;
    UINT uiFilePathLen = strlen(pcFilePath);

    pcExtName = FILE_GetExternNameFromPath(pcFilePath, uiFilePathLen);
    if (pcExtName == NULL)
    {
        return pcFilePath;
    }

    pcExtName --; /* 移动到'.'的位置 */
    *pcExtName = '\0';

    return pcFilePath;
}

CHAR * FILE_ChangeExternName(CHAR *file_name, CHAR *ext_name)
{
    CHAR *pcExtName;
    UINT uiFilePathLen = strlen(file_name);

    pcExtName = FILE_GetExternNameFromPath(file_name, uiFilePathLen);
    if (pcExtName == NULL)
    {
        pcExtName = file_name + uiFilePathLen;
        *pcExtName = '.';
        pcExtName ++;
    }

    strcpy(pcExtName, ext_name);

    return file_name;
}

/* 根据文件路径获取深度, 要求Unix体系的路径格式 */
UINT FILE_GetPathDeep(IN CHAR *pcFilePath, IN UINT uiPathLen)
{
    UINT uiDeep = 0;
    CHAR *pcEnd = pcFilePath + uiPathLen;
    CHAR *pcPtr = pcFilePath;

    if (*pcPtr == '/') /* 根不做计数 */
    {
        pcPtr ++;
    }

    while (pcPtr < pcEnd)
    {
        if (*pcPtr == '/')
        {
            uiDeep ++;
        }

        pcPtr++;
    }

    return uiDeep;
}

/***************************************************
 Description  : 判断文件是否存在
 Return       : 存在: TRUE
                不存在: FALSE
****************************************************/
BOOL_T FILE_IsFileExist(IN CHAR *pcFilePath)
{

    /*
        access函数的Mode参数:
        06     检查读写权限
        04     检查读权限
        02     检查写权限
        01     检查执行权限
        00     检查文件的存在性

        amode参数为0时表示检查文件的存在性，如果文件存在，返回0，不存在，返回-1
    */
    if (0 == access(pcFilePath, 0))
    {
        return TRUE;
    }

    return FALSE;
}

/*****************************************************************************
  判断一个指定路径的目录是否存在
*****************************************************************************/
BOOL_T FILE_IsDirExist(IN CHAR *pcDirName)
{
    /* 局部变量定义 */
    struct stat stBuf;
    BOOL_T bRet;
    INT iRet;

    BS_DBGASSERT(NULL != pcDirName);

    /* 以只读方式打开pcDirName指向的目录 */
    iRet = stat(pcDirName, &stBuf);
    if (0 == iRet)
    {
        bRet = S_ISDIR(stBuf.st_mode);
    }
    else /* 打开文件失败 */
    {
        bRet = FALSE;
    }

    return bRet;    
}


/***************************************************
 Description  : 判断给定路径是否目录
 Input        : pcPath: 路径
 Output       : None
 Return       : _HTTPD_PLUG_RET_E
 Caution      : None
****************************************************/
BOOL_T FILE_IsDir(IN CHAR *pcPath)
{
    struct stat f_stat;
    CHAR szPath[FILE_MAX_PATH_LEN + 1];

    if (NULL == pcPath)
    {
        return FALSE;
    }

    TXT_Strlcpy(szPath, pcPath, sizeof(szPath));
    FILE_PATH_TO_HOST(szPath);

    if (stat (szPath, &f_stat ) == -1)
    {
        return FALSE;
    }

    if (S_IFDIR & f_stat.st_mode)
    {
        return TRUE;
    }

    return FALSE;
}

BS_STATUS FILE_GetSize(IN CHAR *pszFileName, OUT UINT64 *puiFileSize)
{
    struct stat f_stat;
    CHAR szPath[FILE_MAX_PATH_LEN + 1];

    if ((NULL == pszFileName) || (NULL == puiFileSize)) {
        RETURN(BS_NULL_PARA);
    }

    TXT_Strlcpy(szPath, pszFileName, sizeof(szPath));
    FILE_PATH_TO_HOST(szPath);

    if (stat (szPath, &f_stat ) == -1) {
        RETURN(BS_CAN_NOT_OPEN);
    }

    *puiFileSize = (UINT)f_stat.st_size; 

    return BS_OK;
}

BS_STATUS FILE_GetUtcTime
(
    IN CHAR *pszFileName,
    OUT time_t *pulCreateTime,   /* NULL表示不关心 */
    OUT time_t *pulModifyTime,   /* NULL表示不关心 */
    OUT time_t *pulAccessTime    /* NULL表示不关心 */
)
{
    struct stat f_stat; 
    CHAR szPath[FILE_MAX_PATH_LEN + 1];

    if (NULL == pszFileName)
    {
        RETURN(BS_NULL_PARA);
    }

    TXT_Strlcpy(szPath, pszFileName, sizeof(szPath));
    FILE_PATH_TO_HOST(szPath);

    if (stat (szPath, &f_stat ) == -1)
    {
        RETURN(BS_CAN_NOT_OPEN);
    }

    if (NULL != pulCreateTime)
    {
        *pulCreateTime = f_stat.st_ctime;
    }

    if (NULL != pulModifyTime)
    {
        *pulModifyTime = f_stat.st_mtime;
    }

    if (NULL != pulAccessTime)
    {
        *pulAccessTime = f_stat.st_atime;
    }

    return BS_OK;
}

BS_STATUS FILE_SetUtcTime
(
    IN CHAR *pszFileName,
    IN FILE_TIME_MODE_E eModifyTimeMode,
    IN time_t ulModifyTime,
    IN FILE_TIME_MODE_E eAccessTimeMode,
    IN time_t ulAccessTime
)
{
    struct utimbuf stTime;
    time_t ulOldAccessTime;
    time_t ulOldModifyTime;
    time_t ulNowAccessTime;
    time_t ulNowModifyTime;
    time_t ulNewAccessTime;
    time_t ulNewModifyTime;

    /* 如果有不需要更改的,则现得到原来的信息,用于一会儿再设置回去. */
    if ((eAccessTimeMode == FILE_TIME_MODE_NOTMODIFY) || (eModifyTimeMode == FILE_TIME_MODE_NOTMODIFY))
    {
        if ((eAccessTimeMode == FILE_TIME_MODE_NOTMODIFY) && (eModifyTimeMode == FILE_TIME_MODE_NOTMODIFY))
        {
            return BS_OK;
        }
        
        if (BS_OK != FILE_GetUtcTime(pszFileName, NULL, &ulOldModifyTime, &ulOldAccessTime))
        {
            RETURN(BS_ERR);
        }
    }

    if ((eAccessTimeMode == FILE_TIME_MODE_NOW) || (eModifyTimeMode == FILE_TIME_MODE_NOW))
    {
        if (0 != utime(pszFileName, NULL))
        {
            RETURN(BS_ERR);
        }

        if ((eAccessTimeMode == FILE_TIME_MODE_NOW) && (eModifyTimeMode == FILE_TIME_MODE_NOW))
        {
            return BS_OK;
        }

        if (BS_OK != FILE_GetUtcTime(pszFileName, NULL, &ulNowModifyTime, &ulNowAccessTime))
        {
            RETURN(BS_ERR);
        }
    }

    switch (eAccessTimeMode)
    {
        case FILE_TIME_MODE_NOTMODIFY:
            ulNewAccessTime = ulOldAccessTime;
            break;
        case FILE_TIME_MODE_SET:
            ulNewAccessTime = ulAccessTime;
            break;
        case FILE_TIME_MODE_NOW:
            ulNewAccessTime = ulNowAccessTime;
            break;
        default:
            ulNewAccessTime = 0;
            break;
    }

    switch (eModifyTimeMode)
    {
        case FILE_TIME_MODE_NOTMODIFY:
            ulNewModifyTime = ulOldModifyTime;
            break;
        case FILE_TIME_MODE_SET:
            ulNewModifyTime = ulModifyTime;
            break;
        case FILE_TIME_MODE_NOW:
            ulNewModifyTime = ulNowModifyTime;
            break;
        default:
            ulNewModifyTime = 0;
            break;
    }

    memset(&stTime, 0, sizeof(stTime));
    stTime.actime = ulNewAccessTime;
    stTime.modtime = ulNewModifyTime;

    if (0 != utime(pszFileName, &stTime))
    {
        RETURN(BS_ERR);
    }

    return BS_OK;
}

/* 创建目录, 如果已经存在, BS_ALREADY_EXIST*/
BS_STATUS FILE_MakeDir(char *path)
{
    int ret = mkdir(path, 0777);
    if (ret < 0) {
        if (errno == EEXIST) {
            return BS_ALREADY_EXIST;
        }
    }

    return ret;
}

/* 创建目录, 如果已经存在, 返回OK */
BS_STATUS FILE_MakeDir2(char *path)
{
    int ret = mkdir(path, 0777);
    if (ret < 0) {
        if (errno == EEXIST) {
            return 0;
        }
    }

    return ret;
}

/* 创建路径上的所有目录, 如果已经存在，则返回BS_ALREADY_EXIST */
/* pszPath:  以'/'或'\\'结尾的一个路径, 如果不是,则自动忽略最后一个'/'或'\\'之后的字符 */
BS_STATUS FILE_MakeDirs(IN CHAR *pszPath)
{
    CHAR *pcSplit, *pszDir;
    CHAR szPath[FILE_MAX_PATH_LEN + 1];
    INT lRet = BS_OK;

    BS_DBGASSERT(NULL != pszPath);

    if (strlen(pszPath) > FILE_MAX_PATH_LEN)
    {
        RETURN(BS_TOO_LONG);
    }

    TXT_Strlcpy(szPath, pszPath, sizeof(szPath));

    FILE_PATH_TO_UNIX(szPath);

    pszDir = szPath;
    while (NULL != (pcSplit = strchr(pszDir, '/')))
    {
        *pcSplit = '\0';
        lRet = mkdir(szPath, 0777);
        *pcSplit = '/';
        pszDir = pcSplit + 1;
    }

    if (lRet  == EEXIST)
    {
        RETURN(BS_ALREADY_EXIST);
    }

    return BS_OK;
}

/* 创建路径上的所有目录, 如果已经存在，也返回OK */
BS_STATUS FILE_MakePath(IN CHAR *pszPath)
{
    CHAR *pcSplit, *pszDir;
    CHAR szPath[FILE_MAX_PATH_LEN + 1];
    int ret = 0;

    BS_DBGASSERT(NULL != pszPath);

    if (strlen(pszPath) > FILE_MAX_PATH_LEN) {
        RETURN(BS_TOO_LONG);
    }

    TXT_Strlcpy(szPath, pszPath, sizeof(szPath));

    FILE_PATH_TO_UNIX(szPath);

    pszDir = szPath;
    while (NULL != (pcSplit = strchr(pszDir, '/'))) {
        *pcSplit = '\0';
        if (szPath[0]) {
            ret = FILE_MakeDir2(szPath);
            if (ret != 0) {
                break;
            }
        }
        *pcSplit = '/';
        pszDir = pcSplit + 1;
    }

    return FILE_MakeDir2(pszPath);
}

BS_STATUS FILE_MakeFile(IN CHAR *pszFilePath)
{
    FILE * fp;

    fp = FILE_Open(pszFilePath, TRUE, "ab+");
    if (NULL == fp)
    {
        return BS_ERR;
    }
    fclose(fp);

    return BS_OK;
}

BOOL_T FILE_DelFile(IN CHAR *pszFilePath)
{
    if (0 == remove(pszFilePath))
    {
        return TRUE;
    }

    return FALSE;
}

BS_STATUS FILE_DelDir(IN CHAR *pszPath)
{
    CHAR szPath[FILE_MAX_PATH_LEN + 1];
    INT ulLen;
    CHAR *pszName;
    CHAR szSubPath[FILE_MAX_PATH_LEN + 1];

    TXT_Strlcpy(szPath, pszPath, sizeof(szPath));

    ulLen = strlen(szPath);

    FILE_PATH_TO_UNIX(szPath);

    if (szPath[ulLen-1] != '/')
    {
        szPath[ulLen] = '/';
        szPath[ulLen+1] = '\0';
    }

#ifdef IN_WINDOWS
    TXT_Strlcat(szPath, "*", sizeof(szPath));
#endif

    FILE_PATH_TO_HOST(szPath);

    DIR_SCAN_START(szPath, pszName)
    {
        TXT_Strlcpy(szSubPath, szPath, sizeof(szSubPath));
#ifdef IN_WINDOWS
        szSubPath[strlen(szSubPath)-1] = '\0';
#endif
		TXT_Strlcat(szSubPath, pszName, sizeof(szSubPath));
        FILE_DelDir(szSubPath);
    }
    DIR_SCAN_END();

    FILE_SCAN_START(szPath, pszName)
    {
        TXT_Strlcpy(szSubPath, szPath, sizeof(szSubPath));
#ifdef IN_WINDOWS
		szSubPath[strlen(szSubPath)-1] = '\0';
#endif
        TXT_Strlcat(szSubPath, pszName, sizeof(szSubPath));
        FILE_DelFile(szSubPath);
    }
    FILE_SCAN_END();

    TXT_Strlcpy(szPath, pszPath, sizeof(szPath));
    FILE_PATH_TO_HOST(szPath);

    if (0 != rmdir(pszPath))
    {
        RETURN(BS_ERR);
    }

    return BS_OK;
}

/* 先创建文件所在目录路径，再打开文件 */
FILE * FILE_Open(IN CHAR *pszFilePath, IN BOOL_T bIsCreateDirIfNotExist, IN CHAR *pszOpenMode)
{
    CHAR szPath[FILE_MAX_PATH_LEN + 1];

    if (strlen(pszFilePath) > FILE_MAX_PATH_LEN)
    {
        return NULL;
    }

    if (bIsCreateDirIfNotExist == TRUE)
    {
        FILE_MakeDirs (pszFilePath);
    }

    TXT_Strlcpy(szPath, pszFilePath, sizeof(szPath));
    FILE_PATH_TO_HOST(szPath);
    
    return FOPEN(szPath, pszOpenMode);
}

VOID FILE_Close(IN FILE *fp)
{
    fclose(fp);
}

UINT FILE_Read(IN FILE *fp, OUT UCHAR *pucBuf, IN UINT uiBufSize)
{
    return fread(pucBuf, 1, uiBufSize, fp);
}

BOOL_T FILE_IsHaveUtf8Bom(IN FILE *fp)
{
    UCHAR aucBom[3];
    UINT uiReadLen;

    uiReadLen = FILE_Read(fp, aucBom, sizeof(aucBom));
    fseek(fp, 0, SEEK_SET);

    if (uiReadLen < 3)
    {
        return FALSE;
    }

    if ((aucBom[0] == 0xEF) && (aucBom[1] == 0xBB) && (aucBom[2] == 0xBF))
    {
        return TRUE;
    }

    return FALSE;
}

BS_STATUS FILE_CopyTo(IN CHAR *pszSrcFileName, IN CHAR *pszDstFileName)
{
    FILE *fp, *fq;
    UCHAR ucChar;
    time_t ulModifyTime;
    time_t ulAccessTime;
    
    BS_DBGASSERT(NULL != pszSrcFileName);
    BS_DBGASSERT(NULL != pszDstFileName);

    fp = FILE_Open(pszSrcFileName, FALSE, "rb+");
    if (NULL == fp)
    {
        RETURN(BS_CAN_NOT_OPEN);
    }

    fq = FILE_Open(pszDstFileName, TRUE, "wb+");
    if (fq == NULL)
    {
        fclose(fp);
        RETURN(BS_CAN_NOT_OPEN);
    }

    for (ucChar = fgetc(fp); !feof(fp); ucChar = fgetc(fp))
    {
        fputc(ucChar, fq);
    }

    fclose(fp);
    fclose(fq);

    FILE_GetUtcTime(pszSrcFileName, NULL, &ulModifyTime, &ulAccessTime);
    FILE_SetUtcTime(pszDstFileName, FILE_TIME_MODE_SET, ulModifyTime, FILE_TIME_MODE_SET, ulAccessTime);

    return BS_OK;
}

BS_STATUS FILE_MoveTo(IN CHAR *pszSrcFileName, IN CHAR *pszDstFileName, IN BOOL_T bOverWrite)
{
    CHAR szBakFileName[FILE_MAX_PATH_LEN + 1];
    BS_DBGASSERT(NULL != pszSrcFileName);
    BS_DBGASSERT(NULL != pszDstFileName);

    if (strlen(pszDstFileName) > FILE_MAX_PATH_LEN - 4)
    {
        RETURN(BS_ERR);
    }

    if (bOverWrite == TRUE)
    {
        /* 保存备份 */
        TXT_Strlcpy(szBakFileName, pszDstFileName, sizeof(szBakFileName));
        TXT_Strlcat(szBakFileName, ".cnjia1", sizeof(szBakFileName));
        FILE_DelFile(szBakFileName);
        rename(pszDstFileName, szBakFileName);
    }

    /* 移动文件 */
    if (0 != rename(pszSrcFileName, pszDstFileName))
    {
        if (bOverWrite == TRUE)
        {
            /* 失败,恢复原来的文件 */
            rename(szBakFileName, pszDstFileName);
        }

        RETURN(BS_CAN_NOT_OPEN);
    }

    if (bOverWrite == TRUE)
    {
        FILE_DelFile(szBakFileName);
    }

    return BS_OK;
}

VOID FILE_WriteStr(IN FILE *fp, IN CHAR *pszString)
{
    fwrite(pszString, 1, strlen(pszString), fp);
}

/* 读取文件内容放到内存中. 调用者需要释放内存 */
FILE_MEM_S * FILE_Mem(IN CHAR *pszFilePath)
{
    UCHAR *pucFileContext;
    FILE *fp;
    UINT64 uiFileSize;
    FILE_MEM_S *pstMemMap;

    if (! FILE_IsFileExist(pszFilePath)) {
        return NULL;
    }

    if (BS_OK != FILE_GetSize(pszFilePath, &uiFileSize))
    {
        return NULL;
    }

    pstMemMap = MEM_ZMalloc(sizeof(FILE_MEM_S));
    if (NULL == pstMemMap)
    {
        return NULL;
    }

    pucFileContext = MEM_Malloc((ULONG)uiFileSize + 1);
    if (NULL == pucFileContext)
    {
        MEM_Free(pstMemMap);
        return NULL;
    }

    fp = FILE_Open(pszFilePath, FALSE, "rb");
    if (NULL == fp)
    {
        MEM_Free(pstMemMap);
        MEM_Free(pucFileContext);
        return NULL;
    }

    if (uiFileSize != fread(pucFileContext, 1, (UINT)uiFileSize, fp)) {
        fclose(fp);
        MEM_Free(pstMemMap);
        MEM_Free(pucFileContext);
        return NULL;
    }

    pucFileContext[uiFileSize] = '\0';

    fclose(fp);

    pstMemMap->pucFileData = pucFileContext;
    pstMemMap->uiFileLen = uiFileSize;    

    return pstMemMap;    
}

VOID FILE_MemFree(IN FILE_MEM_S *pstMemMap)
{
    if (NULL != pstMemMap)
    {
        if (NULL != pstMemMap->pucFileData)
        {
            MEM_Free(pstMemMap->pucFileData);
        }
        MEM_Free(pstMemMap);
    }
}

/* 将文件内容拷贝到指定缓冲区 */
UINT FILE_MemTo(IN CHAR *pszFilePath, OUT UCHAR *pucBuf, IN UINT uiBufLen/* 0表示不限制长度 */)
{
    FILE *fp;
    UINT64 uiFileSize;
    UINT uiReadLen;

    if (BS_OK != FILE_GetSize(pszFilePath, &uiFileSize))
    {
        return 0;
    }

    if (uiBufLen == 0)
    {
        uiReadLen = MIN(uiBufLen, (UINT)uiFileSize);
    }
    else
    {
        uiReadLen = (UINT)uiFileSize;
    }

    fp = FILE_Open(pszFilePath, FALSE, "rb");
    if (NULL == fp)
    {
        return 0;
    }

    if (uiReadLen != fread(pucBuf, 1, uiReadLen, fp)) {
        fclose(fp);
        return 0;
    }

    fclose(fp);

    return uiReadLen;
}

typedef struct
{
    CHAR szUserScanDir[FILE_MAX_PATH_LEN + 1];    /* 用户要扫描的目录 */
    CHAR *pcPattern; /* NULL表示所有文件类型 */
    PF_ScanFile_Output pfOutput;
    VOID *pUserData;
    CHAR szCurrentScanDir[FILE_MAX_PATH_LEN + 1]; /* 当前正在扫描的目录, 相对于UserScanDir的相对路径 */
    CHAR szScanPattern[FILE_MAX_PATH_LEN + 1]; /* 当前正在扫描的目录, 包含UserScanDir, 包含模式 */
}FILE_SCANFILE_S;

static VOID file_ScanFile(IN FILE_SCANFILE_S *pstScanFile)
{
    CHAR *pcFileName;
    CHAR *pcSubDirName;
    UINT uiCurrentScanDirLen;

    uiCurrentScanDirLen = strlen(pstScanFile->szCurrentScanDir);

    if (uiCurrentScanDirLen != 0)
    {
    	if (snprintf(pstScanFile->szScanPattern, sizeof(pstScanFile->szScanPattern), "%s/%s%s",
                    pstScanFile->szUserScanDir, pstScanFile->szCurrentScanDir, pstScanFile->pcPattern) < 0) {
            return;
        }
    }
    else
    {
        if (snprintf(pstScanFile->szScanPattern, sizeof(pstScanFile->szScanPattern),
                    "%s/%s", pstScanFile->szUserScanDir, pstScanFile->pcPattern) < 0) {
            return;
        }
    }

    FILE_SCAN_START(pstScanFile->szScanPattern, pcFileName)
	{
        pstScanFile->pfOutput(pstScanFile->szCurrentScanDir, pcFileName, pstScanFile->pUserData);
	}FILE_SCAN_END();

    if (uiCurrentScanDirLen == 0)
    {
        if (snprintf(pstScanFile->szScanPattern, sizeof(pstScanFile->szScanPattern),
                    "%s/*", pstScanFile->szUserScanDir) < 0) {
            return;
        }
    }
    else
    {
        if (snprintf(pstScanFile->szScanPattern, sizeof(pstScanFile->szScanPattern),
                    "%s/%s*", pstScanFile->szUserScanDir, pstScanFile->szCurrentScanDir) < 0) {
            return;
        }
    }

	DIR_SCAN_START(pstScanFile->szScanPattern, pcSubDirName)
	{
        sprintf(&pstScanFile->szCurrentScanDir[uiCurrentScanDirLen], "%s/", pcSubDirName);
		file_ScanFile(pstScanFile);
	}DIR_SCAN_END();

    return;
}

VOID FILE_ScanFile
(
    IN CHAR *pcScanDir,
    IN CHAR *pcPattern, /* NULL表示所有文件类型 */
    IN PF_ScanFile_Output pfOutput,
    IN VOID *pUserData
)
{
    FILE_SCANFILE_S stScanFile;
    UINT uiScanDirLen;

    if (NULL == pcPattern)
    {
        pcPattern = "*";
    }

    uiScanDirLen = strlen(pcScanDir);
    if (uiScanDirLen == 0)
    {
        BS_DBGASSERT(0);
        return;
    }

    memset(&stScanFile, 0, sizeof(stScanFile));
    strlcpy(stScanFile.szUserScanDir, pcScanDir, sizeof(stScanFile.szUserScanDir));
    stScanFile.pcPattern = pcPattern;
    stScanFile.pfOutput = pfOutput;
    stScanFile.pUserData = pUserData;
    stScanFile.szCurrentScanDir[0] = '\0';

    if (stScanFile.szUserScanDir[uiScanDirLen - 1] == '/')
    {
        stScanFile.szUserScanDir[uiScanDirLen - 1] = '\0';
    }

    file_ScanFile(&stScanFile);

    return;
}

/* 返回line以\0结尾
   返回应该读取多长,当ret >= size时表示了截断,且下次读取下一行,这和fgets是有区别的
   返回<=0则表示结束 */
int FILE_ReadLine(FILE *fp, char *line, int size, char end_char)
{
    char *tmp = line;
    int count = 0;
    int c;

    BS_DBGASSERT(size > 0);

    line[size - 1] = 0;

    do {
        c = fgetc(fp);
        if( feof(fp) ) {
            break ;
        }
        count ++;
        if (count < size) {
            *tmp = c;
            tmp ++;
        }
    }while(c != end_char);

    if (count < size) {
        line[count] = 0;
    }

    return count;
}

