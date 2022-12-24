/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-7-9
* Description: 
* History:     
******************************************************************************/

#ifndef __FILE_UTL_H_
#define __FILE_UTL_H_

#include <stdbool.h>
#include "utl/cjson.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

/* ---defines--- */
#define FILE_MAX_PATH_LEN 511

#ifdef IN_WINDOWS
#define S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m)	(((m) & S_IFMT) == S_IFSOCK)
#endif

typedef struct
{
    UINT64 uiFileLen;   /* 文件长度 */
    UCHAR *pucFileData; /* 文件数据. 其内存长度比文件长度多1,最后添了一个'\0' */
}FILE_MEM_S;

#ifdef IN_WINDOWS

static inline FILE * FOPEN(IN CHAR *pcFileName, IN CHAR *pcMode)
{
    FILE *fp;

    fp = _fsopen(pcFileName, pcMode, _SH_DENYWR);

    return fp;
}

#define FILE_SCAN_START(pcPath,pcFileName)    \
    do {    \
        struct _finddata_t   _c_file;  \
        long _hFile;                 \
        UINT   _ulRet = 0;          \
        for (_hFile = _findfirst(pcPath, &_c_file); ((_hFile != -1L) && (_ulRet == 0)); _ulRet = _findnext(_hFile, &_c_file)) \
        {   \
            if ((!(_c_file.attrib & _A_SUBDIR)) && (strcmp(_c_file.name, ".") != 0) && (strcmp(_c_file.name, "..") != 0)) \
            {   \
                pcFileName = _c_file.name;

#define FILE_SCAN_END()      }}_findclose(_hFile);}while(0)

#define DIR_SCAN_START(pcPath,pcDirName)    \
    do {    \
        struct _finddata_t _c_file;  \
        long _hFile;                 \
        UINT   _ulRet = 0;          \
        for (_hFile = _findfirst(pcPath, &_c_file); ((_hFile != -1L) && (_ulRet == 0)); _ulRet = _findnext(_hFile, &_c_file)) \
        {   \
            if ((_c_file.attrib & _A_SUBDIR) && (strcmp(_c_file.name, ".") != 0) && (strcmp(_c_file.name, "..") != 0)) \
            {   \
                pcDirName = _c_file.name;

#define DIR_SCAN_END()      }}_findclose(_hFile);}while(0)

#define FILE_PATH_TO_HOST(pszFilePath)  \
	do {  \
		CHAR *_chc;	\
		_chc = pszFilePath;	\
		while(*_chc != '\0')	\
		{	\
			if (*_chc == '/')	\
			{	\
				*_chc = '\\';	\
			}	\
			_chc++;	\
		}	\
	}while(0)

#define FILE_GET_CURRENT_DIRECTORY(pszBuf,ulBufLen) _getcwd(pszBuf, ulBufLen)

#endif

#ifdef IN_UNIXLIKE

#define FOPEN(pcFileName, pcMode) fopen(pcFileName, pcMode)

#define FILE_SCAN_START(pcPath,pcFileName)    \
    do { \
        struct dirent *_pstResult = NULL; \
        DIR *_pstDir = opendir(pcPath);  \
        while(_pstDir)  { \
            _pstResult = readdir(_pstDir); \
            if (_pstResult == NULL) {   \
                break;  \
            }   \
            if (_pstResult->d_type & DT_DIR) { \
                continue;  \
            }   \
            pcFileName = _pstResult->d_name;

#define FILE_SCAN_END()      } if (_pstDir) {closedir(_pstDir);}}while(0)

#define DIR_SCAN_START(_pcPath,_pcDirName)    \
    do { \
        struct dirent *_pstResult = NULL; \
        DIR *_pstDir = opendir(_pcPath);  \
        while(_pstDir)  { \
            _pstResult = readdir(_pstDir); \
            if (_pstResult == NULL) {   \
                break;  \
            }   \
            if ((_pstResult->d_type & DT_DIR) == 0) {   \
                continue;  \
            }   \
            if ((strcmp(_pstResult->d_name, ".") == 0) || (strcmp(_pstResult->d_name, "..") == 0)) { \
                continue;  \
            }   \
            _pcDirName = _pstResult->d_name;


#define DIR_SCAN_END()  } if (_pstDir){closedir(_pstDir);}}while(0)

#define FILE_PATH_TO_HOST(pszFilePath)  \
    do {  \
        CHAR *_chc;	\
        _chc = pszFilePath;	\
    	while(*_chc != '\0')	\
    	{	\
    		if (*_chc == '\\')	\
    		{	\
    			*_chc = '/';	\
    		}	\
    		_chc++;	\
    	}	\
    }while(0)

#define FILE_GET_CURRENT_DIRECTORY(pszBuf,ulBufLen) getcwd(pszBuf, ulBufLen)

#endif

#define FILE_SET_CURRENT_DIRECTORY(szPath) chdir(szPath)


#define FILE_PATH_TO_UNIX(pszFilePath)  \
    do {  \
        CHAR *_chc; \
        _chc = pszFilePath; \
        while(*_chc != '\0')    \
        {   \
            if (*_chc == '\\')   \
            {   \
                *_chc = '/';   \
            }   \
            _chc++; \
        }   \
    }while(0)


typedef enum
{
    FILE_TIME_MODE_NOTMODIFY,
    FILE_TIME_MODE_NOW,
    FILE_TIME_MODE_SET
}FILE_TIME_MODE_E;

/* ---funcs--- */
extern BS_STATUS FILE_GetSize(IN CHAR *pszFileName, OUT UINT64 *puiFileSize);
extern BOOL_T FILE_IsFileExist(IN CHAR *pcFilePath);
extern BOOL_T FILE_IsDirExist(IN CHAR *pcDirName);
extern BOOL_T FILE_IsDir(IN CHAR *pcPath);
/* 先创建文件所在目录路径，再打开文件 */
extern FILE * FILE_Open(IN CHAR *pszFilePath, IN BOOL_T bIsCreateDirIfNotExist, IN CHAR *pszOpenMode);
extern VOID FILE_Close(IN FILE *fp);
extern UINT FILE_Read(IN FILE *fp, OUT UCHAR *pucBuf, IN UINT uiBufSize);
extern BOOL_T FILE_IsHaveUtf8Bom(IN FILE *fp);
/* 如果已经存在，则返回BS_ALREADY_EXIST */
/*pszPath:  以'/'或'\\'结尾的一个路径, 如果不是,则自动忽略最后一个'/'或'\\'之后的字符*/
extern BS_STATUS FILE_MakeDirs(IN CHAR *pszPath);
extern BS_STATUS FILE_MakePath(IN CHAR *pszPath);
extern BS_STATUS FILE_MakeFile(IN CHAR *pszFilePath);
extern BS_STATUS FILE_DelDir(IN CHAR *pszPath);
extern BOOL_T FILE_DelFile(IN CHAR *pszFilePath);
extern BS_STATUS FILE_GetUtcTime
(
    IN CHAR *pszFileName,
    OUT time_t *pulCreateTime,   /* NULL表示不关心 */
    OUT time_t *pulModifyTime,   /* NULL表示不关心 */
    OUT time_t *pulAccessTime    /* NULL表示不关心 */
);
extern BS_STATUS FILE_SetUtcTime
(
    IN CHAR *pszFileName,
    IN FILE_TIME_MODE_E eModifyTimeMode,
    IN time_t ulModifyTime,
    IN FILE_TIME_MODE_E eAccessTimeMode,
    IN time_t ulAccessTime
);
extern CHAR * FILE_GetFileNameFromPath(IN CHAR *pszPath);

extern BS_STATUS FILE_GetFileNameWithOutExtFromPath(IN CHAR *pszPath, OUT LSTR_S *pstFileNameWithOutExt);

extern BOOL_T FILE_IsAbsolutePath(IN CHAR *pszPath);

extern char * FILE_ToAbsPath(char *base_dir, char *path, OUT char *dst, int dst_size);

extern char * FILE_Dup2AbsPath(IN char *base_dir, IN char *path);

/* 从携带文件名的路径中,把路径取出来,路径以"/"结束 */
VOID FILE_GetPathFromFilePath(CHAR *pszFilePath, OUT CHAR *szPath);

extern CHAR * FILE_GetExternNameFromPath(IN CHAR *pszPath, IN UINT uiPathLen);

/* 抹去扩展名 */
extern CHAR * FILE_EarseExternName(IN CHAR *pcFilePath);

extern CHAR * FILE_ChangeExternName(CHAR *file_name, CHAR *ext_name);

extern UINT FILE_GetPathDeep(IN CHAR *pcFilePath, IN UINT uiPathLen);

extern BS_STATUS FILE_CopyTo(IN CHAR *pszSrcFileName, IN CHAR *pszDstFileName);

extern BS_STATUS FILE_MoveTo(IN CHAR *pszSrcFileName, IN CHAR *pszDstFileName, IN BOOL_T bOverWrite);

extern VOID FILE_WriteStr(IN FILE *fp, IN CHAR *pszString);

/* 读取文件内容放到内存中. 调用者需要释放内存 */
extern FILE_MEM_S * FILE_Mem(IN CHAR *pszFilePath);
extern VOID FILE_MemFree(IN FILE_MEM_S *pstMemMap);

/* 将文件内容拷贝到指定缓冲区 */
extern UINT FILE_MemTo(IN CHAR *pszFilePath, OUT UCHAR *pucBuf, IN UINT uiBufLen/* 0表示不限制长度 */);

typedef VOID (*PF_ScanFile_Output)
(
    IN CHAR *pcCurrentScanDir, /* 文件所在的相对于UserScanDir的相对目录 */
    IN CHAR *pcFileName,
    IN VOID *pUserData
);

VOID FILE_ScanFile
(
    IN CHAR *pcScanDir,
    IN CHAR *pcPattern, /* NULL表示所有文件类型 */
    IN PF_ScanFile_Output pfOutput,
    IN VOID *pUserData
);

int FILE_ReadLine(FILE *fp, char *line, int size, char end_char);

cJSON *FILE_LoadJson(const char *filename, bool is_encrypted);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__FILE_UTL_H_*/


