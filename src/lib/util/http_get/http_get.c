/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-10-20
* Description: 
* History:     
******************************************************************************/
/* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_HTTPGET
    
#include "bs.h"

#include "utl/file_utl.h"
#include "utl/socket_utl.h"
#include "utl/time_utl.h"
#include "utl/http_lib.h"

static BS_STATUS httpget_WriteFile(IN UCHAR *pucData, IN UINT uiDataLen, IN USER_HANDLE_S *pstUserPointer)
{
    FILE *fp = pstUserPointer->ahUserHandle[0];

    fwrite(pucData, 1, uiDataLen, fp);

    return BS_OK;
}

static BS_STATUS httpget_ProcBody
(
    IN INT iSocketId,
    IN HTTP_HEAD_PARSER hHeadParser,
    IN UCHAR *pucData,
    IN UINT uiDataLen,
    IN CHAR *pcSaveFile
)
{
    FILE *fp;
    HTTP_BODY_PARSER hBodyParser;
    UCHAR aucData[512];
    UINT uiReadLen;
    UINT uiParsedLen;
    BS_STATUS eRet;
    USER_HANDLE_S stUserHandle;

    fp = FILE_Open(pcSaveFile, TRUE, "wb+");
    if (NULL == fp)
    {
        RETURN(BS_ERR);
    }

    stUserHandle.ahUserHandle[0] = fp;
    hBodyParser = HTTP_BODY_CreateParser(hHeadParser, httpget_WriteFile, &stUserHandle);
    if (NULL == hBodyParser)
    {
        fclose(fp);
        FILE_DelFile(pcSaveFile);
        RETURN(BS_ERR);
    }

    if (HTTP_BODY_IsFinish(hBodyParser))
    {
        HTTP_BODY_DestroyParser(hBodyParser);
        fclose(fp);
        return BS_OK;
    }

    if (uiDataLen > 0)
    {
        eRet = HTTP_BODY_Parse(hBodyParser, pucData, uiDataLen, &uiParsedLen);
        if (eRet == BS_OK)
        {
            HTTP_BODY_DestroyParser(hBodyParser);
            fclose(fp);
            return BS_OK;
        }
        else if (eRet != BS_NOT_COMPLETE)
        {
            HTTP_BODY_DestroyParser(hBodyParser);
            fclose(fp);
            FILE_DelFile(pcSaveFile);
            RETURN(BS_ERR);
        }
    }

    do {
        eRet = Socket_Read2(iSocketId, aucData, sizeof(aucData), &uiReadLen, 0);
        if ((BS_OK != eRet) && (BS_PEER_CLOSED != eRet))
        {
            HTTP_BODY_DestroyParser(hBodyParser);
            fclose(fp);
            FILE_DelFile(pcSaveFile);
            RETURN(BS_ERR);
        }

        eRet = HTTP_BODY_Parse(hBodyParser, aucData, uiReadLen, &uiParsedLen);
        if (eRet == BS_OK)
        {
            break;
        }
    }while(eRet == BS_NOT_COMPLETE);

    if (eRet == BS_NOT_COMPLETE)
    {
        eRet = BS_OK;
    }

    HTTP_BODY_DestroyParser(hBodyParser);
    fclose(fp);

    if (eRet != BS_OK)
    {
        FILE_DelFile(pcSaveFile);
    }

    return eRet;
}

/* BS_ALREADY_EXIST:不用更新; BS_OK:更新成功; 其他:失败 */
BS_STATUS HTTPGET_GetFile
(
    IN CHAR *pszServer,
    IN USHORT usPort, /* 主机序 */
    IN CHAR *pszPath,
    IN time_t ulOldFileTime, /* 原来文件的时间. 如果服务器上的文件时间和这个不同,则下载 */
    IN CHAR *pszSaveAsFile  /* 如果为NULL, 则使用取到的文件名 */
)
{
    static CHAR * pszGetRequest =
        "GET %s HTTP/1.0\r\n"
        "Accept: */*\r\n"
        "If-Modified-Since: %s GMT\r\n"
        "Host: %s\r\n"
        "\r\n";
    CHAR szRequest[256];
    CHAR szStringTime[128];
    BS_STATUS eRet;
    UINT ulDataLen;
    CHAR *pszTime;
    time_t ulUtcTime;
    UINT ulIp;
    UCHAR aucData[2048];
    UINT uiReadSize;
    UINT ulHeadLen = 0;
    HTTP_HEAD_PARSER hHeadParser;
    HTTP_STATUS_CODE_E eStatusCode;
    INT iSocketId;

    if (NULL == pszSaveAsFile)
    {
        pszSaveAsFile = FILE_GetFileNameFromPath(pszPath);
    }

    (VOID)TM_Utc2String(ulOldFileTime, szStringTime);

    /* 发送HTTP请求 */
    snprintf(szRequest, sizeof(szRequest), pszGetRequest, pszPath, szStringTime, pszServer);

    ulIp = Socket_NameToIpHost(pszServer);

    iSocketId = Socket_Create(AF_INET, SOCK_STREAM);
    if (iSocketId < 0)
    {
        RETURN(BS_ERR);
    }

    if (BS_OK != Socket_Connect(iSocketId, ulIp, usPort))
    {
        Socket_Close(iSocketId);
        RETURN(BS_ERR);
    }

    if (BS_OK != Socket_WriteUntilFinish(iSocketId, (VOID*)szRequest, strlen(szRequest), 0))
    {
        Socket_Close(iSocketId);
        RETURN(BS_ERR);
    }

    ulDataLen = 0;
    do  {
        eRet = Socket_Read2(iSocketId, aucData, sizeof(aucData), &uiReadSize, 0);
        if (BS_OK != eRet)
        {
            Socket_Close(iSocketId);
            RETURN(BS_ERR);
        }

        ulDataLen += uiReadSize;
        
    } while(0 == (ulHeadLen = HTTP_GetHeadLen((CHAR*)aucData, ulDataLen)));

    if (eRet != BS_OK)
    {
        Socket_Close(iSocketId);
        RETURN(BS_ERR);
    }

    hHeadParser = HTTP_CreateHeadParser();
    if (NULL == hHeadParser)
    {
        Socket_Close(iSocketId);
        RETURN(BS_ERR);
    }

    if (BS_OK != HTTP_ParseHead(hHeadParser, (CHAR*)aucData, ulHeadLen, HTTP_RESPONSE))
    {
        HTTP_DestoryHeadParser(hHeadParser);
        Socket_Close(iSocketId);
        RETURN(BS_ERR);
    }

    eStatusCode = HTTP_GetStatusCode(hHeadParser);
    if (eStatusCode == HTTP_STATUS_OK)
    {
        if (BS_OK != httpget_ProcBody(iSocketId, hHeadParser, aucData + ulHeadLen, ulDataLen - ulHeadLen, pszSaveAsFile))
        {
            HTTP_DestoryHeadParser(hHeadParser);
            Socket_Close(iSocketId);
            RETURN(BS_ERR);
        }
    }
    else if (eStatusCode != HTTP_STATUS_NOT_MODI)
    {
        HTTP_DestoryHeadParser(hHeadParser);
        Socket_Close(iSocketId);
        RETURN(BS_ERR);
    }

    if (eStatusCode == HTTP_STATUS_OK)
    {
        pszTime = HTTP_GetHeadField(hHeadParser, HTTP_FIELD_LAST_MODIFIED);
        if (NULL != pszTime)
        {
            if (BS_OK == TM_String2Utc(pszTime, &ulUtcTime))
            {
                FILE_SetUtcTime(pszSaveAsFile, FILE_TIME_MODE_SET, ulUtcTime, FILE_TIME_MODE_SET, ulUtcTime);
            }
        }
    }
    
    HTTP_DestoryHeadParser(hHeadParser);
    Socket_Close(iSocketId);

    if (eStatusCode == HTTP_STATUS_NOT_MODI)
    {
        return BS_ALREADY_EXIST;
    }

    return BS_OK;
}

