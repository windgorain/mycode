/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-10-21
* Description: 
* History:     
******************************************************************************/

#define RETCODE_FILE_NUM RETCODE_FILE_NUM_UPDATEUTL
        
#include "bs.h"
    
#include "utl/file_utl.h"
#include "utl/http_aly.h"
#include "utl/cff_utl.h"
#include "utl/exturl_utl.h"
#include "utl/txt_utl.h"
#include "utl/http_get.h"
#include "utl/update_utl.h"

#ifdef IN_WINDOWS
#define _UPD_UPDATE_FILE_NAME "update.bat"
#endif

#define _UDP_UPDATE_FILE_TMP_EXT_NAME ".upd"


typedef enum
{
    _UPD_UPDATE_MODE_TIME,
    _UPD_UPDATE_MODE_VER,
}_UPD_UPDATE_MODE_E;

static _UPD_UPDATE_MODE_E _UPD_GetModeByModeString(IN CHAR *pszMode)
{
    if (NULL != pszMode)
    {        
        if (strcmp(pszMode, "time") == 0)
        {
            return _UPD_UPDATE_MODE_TIME;
        }
        else if (strcmp(pszMode, "ver") == 0)
        {
            return _UPD_UPDATE_MODE_VER;
        }
    }

    
    return _UPD_UPDATE_MODE_TIME;
}


static BS_STATUS _UPD_UpdateFileByTime
(
    IN CHAR *pszFileName,
    IN CHAR *pszServer,
    IN USHORT usPort,
    IN CHAR *pszUrlPath
)
{
    CHAR szUpdFileName[FILE_MAX_PATH_LEN + 1];
    CHAR szUrlPath[HTTP_ALY_MAX_URL_LEN + 1];
    time_t tTime;
    BS_STATUS eRet;

    TXT_Strlcpy(szUrlPath, pszUrlPath, sizeof(szUrlPath));
    TXT_Strlcat(szUrlPath, pszFileName, sizeof(szUrlPath));

    TXT_Strlcpy(szUpdFileName, pszFileName, sizeof(szUpdFileName));
    TXT_Strlcat(szUpdFileName, _UDP_UPDATE_FILE_TMP_EXT_NAME, sizeof(szUpdFileName));

    if (BS_OK != FILE_GetUtcTime(pszFileName, NULL, &tTime, NULL))
    {
        tTime = 0;
    }

    eRet = HTTPGET_GetFile(pszServer, usPort, szUrlPath, tTime, szUpdFileName);

	return eRet;
}


static BS_STATUS _UPD_UpdateFileByVer
(
    IN HANDLE hOldVerCff,
    IN HANDLE hNewVerCff,
    IN CHAR *pszFileName,
    IN CHAR *pszServer,
    IN USHORT usPort,
    IN CHAR *pszUrlPath
)
{
    CHAR *pcNewVer;
    CHAR *pcOldVer;
    CHAR szUpdFileName[FILE_MAX_PATH_LEN + 1];
    CHAR szUrlPath[HTTP_ALY_MAX_URL_LEN + 1];
    BS_STATUS eRet;

    if (hOldVerCff != 0)
    {
        if (BS_OK != CFF_GetPropAsString(hNewVerCff, pszFileName, "ver", &pcNewVer))
        {
            RETURN(BS_ERR);
        }

        if ((BS_OK == CFF_GetPropAsString(hOldVerCff, pszFileName, "ver", &pcOldVer))
            && (strcmp(pcOldVer, pcNewVer) == 0))
        {
            return BS_ALREADY_EXIST;   
        }
    }

    TXT_Strlcpy(szUrlPath, pszUrlPath, sizeof(szUrlPath));
    TXT_Strlcat(szUrlPath, pszFileName, sizeof(szUrlPath));

    TXT_Strlcpy(szUpdFileName, pszFileName, sizeof(szUpdFileName));
    TXT_Strlcat(szUpdFileName, _UDP_UPDATE_FILE_TMP_EXT_NAME, sizeof(szUpdFileName));

    eRet = HTTPGET_GetFile(pszServer, usPort, szUrlPath, 0, szUpdFileName);

	return eRet;
}


static UPDATE_RET_E _UPD_UpdateByVerFile
(
    IN CHAR *pszOldVerFileName,
    IN CHAR *pszServer,
    IN USHORT usPort,
    IN CHAR *pszUrlPath,
    IN PF_UPDATE_NOTIFY_FUNC pfUpdateNotify,
    IN VOID *pUserHandle
)
{
    CFF_HANDLE hNewVerCff;
    CFF_HANDLE hOldVerCff;
    CHAR *pszSection;
    CHAR *pszMode;
    _UPD_UPDATE_MODE_E eMode;
    BS_STATUS eRet;
    BOOL_T bNeedRestart = FALSE;
    CHAR *pszCmd;
    CHAR szNewVerFileName[FILE_MAX_PATH_LEN + 1];
    CHAR szFileNameUpd[FILE_MAX_PATH_LEN + 1];
    CHAR szFileNameHost[FILE_MAX_PATH_LEN + 1];
    CHAR szFileNameHostOld[FILE_MAX_PATH_LEN + 1];
    UINT uiTotleCount;
    UINT uiCurrentCount;
    UINT uiNeedRestart = 0;
    UPDATE_FILE_S stUpdate = {0};
    BOOL_T bRun;
    UPDATE_FILE_RESULT_E eResult;

    TXT_Strlcpy(szNewVerFileName, pszOldVerFileName, sizeof(szNewVerFileName));
    TXT_Strlcat(szNewVerFileName, _UDP_UPDATE_FILE_TMP_EXT_NAME, sizeof(szNewVerFileName));

    if (NULL == (hNewVerCff = CFF_INI_Open(szNewVerFileName, CFF_FLAG_READ_ONLY)))
    {
        return UPDATE_RET_ERR;
    }

    hOldVerCff = CFF_INI_Open(pszOldVerFileName, CFF_FLAG_CREATE_IF_NOT_EXIST | CFF_FLAG_READ_ONLY);    

    uiTotleCount = CFF_GetTagNum(hNewVerCff);
    uiCurrentCount = 0;

    stUpdate.uiTotleUpdCount = uiTotleCount;

    CFF_SCAN_TAG_START(hNewVerCff, pszSection)
    {
        uiCurrentCount ++;
        stUpdate.uiCurrentUpdCount = uiCurrentCount;
        stUpdate.pcUpdFile = pszSection;

        pfUpdateNotify(UPDATE_EVENT_UPDATE_FILE, &stUpdate, pUserHandle);

        if (BS_OK != CFF_GetPropAsString(hNewVerCff, pszSection, "update-mode", &pszMode))
        {
            pszMode = NULL;
        }

        eMode = _UPD_GetModeByModeString(pszMode);

        switch (eMode)
        {
            case _UPD_UPDATE_MODE_TIME:
                eRet = _UPD_UpdateFileByTime(pszSection, pszServer, usPort, pszUrlPath);
                break;

            case _UPD_UPDATE_MODE_VER:
                eRet = _UPD_UpdateFileByVer(hOldVerCff,
                    hNewVerCff, pszSection, pszServer, usPort, pszUrlPath);
                break;

            default:
                BS_DBGASSERT(0);
            break;
        }

        
        if (BS_OK == eRet)
        {
            TXT_Strlcpy(szFileNameUpd, pszSection, sizeof(szFileNameUpd));
            TXT_Strlcat(szFileNameUpd, _UDP_UPDATE_FILE_TMP_EXT_NAME, sizeof(szFileNameUpd));
            TXT_Strlcpy(szFileNameHost, pszSection, sizeof(szFileNameHost));
            TXT_Strlcpy(szFileNameHostOld, pszSection, sizeof(szFileNameHostOld));
            TXT_Strlcat(szFileNameHostOld, ".old", sizeof(szFileNameHostOld));

            FILE_PATH_TO_HOST(szFileNameUpd);
            FILE_PATH_TO_HOST(szFileNameHost);
            FILE_PATH_TO_HOST(szFileNameHostOld);

            if (BS_OK != CFF_GetPropAsString(hNewVerCff, pszSection, "cmd", &pszCmd))
            {
                pszCmd = NULL;
            }

            if (pszCmd != NULL)
            {
                bRun = TRUE;
            }
            else
            {
                bRun = FALSE;
            }

            if (! FILE_IsFileExist(szFileNameHost))
            {
                FILE_MoveTo(szFileNameUpd, szFileNameHost, TRUE);
            }
            else if (FILE_DelFile(szFileNameHost))
            {
                FILE_MoveTo(szFileNameUpd, szFileNameHost, TRUE);
            }
            else
            {
                FILE_MoveTo(szFileNameHost, szFileNameHostOld, TRUE);
                FILE_MoveTo(szFileNameUpd, szFileNameHost, TRUE);
                bNeedRestart = TRUE;
            }

            if (bRun)
            {
                WinExec(pszCmd, SW_HIDE);
            }
            
            if (BS_OK == CFF_GetPropAsUint(hNewVerCff, pszSection, "restart", &uiNeedRestart)
                && (uiNeedRestart == 1))
            {
                bNeedRestart = TRUE;
            }
        }

        if (BS_OK == eRet)
        {
            eResult = UPDATE_FILE_RESULT_OK;
        }
        else if (BS_ALREADY_EXIST == eRet)
        {
            eResult = UPDATE_FILE_RESULT_IS_NEWEST;
        }
        else
        {
            eResult = UPDATE_FILE_RESULT_FAILED;
        }

        pfUpdateNotify(UPDATE_EVENT_UPDATE_FILE_RESULT, UINT_HANDLE(eResult), pUserHandle);
    }CFF_SCAN_END();

    CFF_Close(hNewVerCff);
    if (hOldVerCff != NULL)
    {
        CFF_Close(hOldVerCff);
    }

    FILE_MoveTo(szNewVerFileName, pszOldVerFileName, TRUE);

    if (bNeedRestart == TRUE)
    {
        return UPDATE_RET_REBOOT;
    }

    return UPDATE_RET_OK;
}

UPDATE_RET_E UPD_Update
(
    IN CHAR * pcUrl,
    IN CHAR *pcSaveFileName,
    IN PF_UPDATE_NOTIFY_FUNC pfUpdateNotify,
    IN VOID *pUserHandle
)
{
    CHAR * pcUrlFileName;
    CHAR szFileName[FILE_MAX_PATH_LEN + 1];
    BS_STATUS eRet = BS_OK;
    UPDATE_RET_E eUpdRet;
	EXTURL_S stExtUrl;
    CHAR *pcSaveTo = pcSaveFileName;
    UPDATE_FILE_S stFile = {0};

	eRet = EXTURL_Parse(pcUrl, &stExtUrl);
	if (eRet != BS_OK)
	{
		return UPDATE_RET_ERR;
	}

	if (stExtUrl.usPort == 0)
	{
		stExtUrl.usPort = 80;
	}

    pcUrlFileName = FILE_GetFileNameFromPath(stExtUrl.szPath);

    if (pcSaveTo == NULL)
    {
        pcSaveTo = pcUrlFileName;
    }

    TXT_Strlcpy(szFileName, pcSaveTo, sizeof(szFileName));
    TXT_Strlcat(szFileName, _UDP_UPDATE_FILE_TMP_EXT_NAME, sizeof(szFileName));

    pfUpdateNotify(UPDATE_EVENT_START, NULL, pUserHandle);
    stFile.pcUpdFile = pcUrlFileName;
    pfUpdateNotify(UPDATE_EVENT_UPDATE_FILE, &stFile, pUserHandle);
    eRet = HTTPGET_GetFile(stExtUrl.szAddress, stExtUrl.usPort, stExtUrl.szPath, 0, szFileName);
    if ((eRet != BS_OK) && (BS_ALREADY_EXIST != eRet))
    {
        pfUpdateNotify(UPDATE_EVENT_UPDATE_FILE_RESULT, (HANDLE)UPDATE_FILE_RESULT_FAILED, pUserHandle);
        eUpdRet = UPDATE_RET_ERR;
    }
    else
    {
        pfUpdateNotify(UPDATE_EVENT_UPDATE_FILE_RESULT, (HANDLE)UPDATE_FILE_RESULT_OK, pUserHandle);
        TXT_Strlcpy(szFileName, pcSaveTo, sizeof(szFileName));
        *pcUrlFileName = '\0';
        eUpdRet = _UPD_UpdateByVerFile(szFileName, stExtUrl.szAddress, stExtUrl.usPort, stExtUrl.szPath, pfUpdateNotify, pUserHandle);
    }

    pfUpdateNotify(UPDATE_EVENT_END, NULL, pUserHandle);

    return eUpdRet;
}

