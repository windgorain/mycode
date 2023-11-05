/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2009-10-18
* Description: 
*   接收的数据格式:
*       TotalLength
*       functionName
*       type;length;context
*       type;length;context
*       ......
*       其中';'为分隔符. 每一行用'\n'分开.注意不是"\r\n".最后以'\n'结束最后一行
*       总长度不包含第一行的长度和第一行的'\n'符号
*       举例如下:
*       ......
*       string;4;test
*       integer;3;234
*       boolean;1;1
*       boolean;1;0
* 
* 回应的格式:
*       TotalLength
*       ScallErrCode;length;retInfo
*       funcReturnType;length;context
*       type;length;context
*       type;length;context
*       ......
*       如果某个参数不是输出参数,则type=void,length=0,context零个字节. 
*       retInfo 可以为"", 这种情况下长度为0
*       ScallErrCode表示SCALL调用是否成功.0: 成功; 
*       funcReturnType为SCALL调用的具体函数的返回值类型.
*       type如果为void, 表示不是输出参数，只是占位用
* History:     
******************************************************************************/

#define RETCODE_FILE_NUM RETCODE_FILE_NUM_PLUG_BASE
    
#include "bs.h"

#include "utl/sem_utl.h"
#include "utl/file_utl.h"
#include "utl/local_info.h"
#include "utl/txt_utl.h"
#include "utl/cff_utl.h"
#include "utl/msgque_utl.h"
#include "utl/string_utl.h"
#include "utl/mbuf_utl.h"

#define _SCALL_DFT_INI_FILE    "scall_config.ini"
#define _SCALL_DFT_SERVER_PORT 65351  

#define _SCALL_MAX_PARAM_NUM    10  

#define _SCALL_LINE_END_CHAR '\n'
#define _SCALL_SPLIT_CHAR ';'

#define _SCALL_EVNET 0x1

#define _SCALL_MSG_TYPE_SSLTCP   1
#define _SCALL_MSG_TYPE_TIMEOUT  2


#define _SCALL_DBG_FLAG_PROCESS 1

#define _SCALL_DBG_PRINT(uiFlag,x) \
    do {    \
        if (uiFlag & g_uiScallDbgFlag) \
        {   \
            IC_Info x;  \
        }   \
    }while (0)


#define _SCALL_SCAN_PARAM_LINE_BEGIN(_pucParams,_eType,_ulParamLen,_pucParamdata)  \
    do {    \
        CHAR *_pcSplit; \
        CHAR *_pszType;  \
        CHAR *_pszLen;  \
        CHAR *_pszStart = _pucParams; \
        while (*_pszStart != '\0')  {   \
            _pcSplit = strchr(_pszStart, _SCALL_SPLIT_CHAR);   \
            if (NULL == _pcSplit){break;}   \
            _pszType = _pszStart;  *_pcSplit = '\0'; _pszStart = _pcSplit + 1;  \
            _eType = _SCALL_GetTypeByTypeName(_pszType);    \
            _pcSplit = strchr(_pszStart, _SCALL_SPLIT_CHAR);   \
            if (NULL == _pcSplit){break;}   \
            _pszLen = _pszStart;  *_pcSplit = '\0'; _pszStart = _pcSplit + 1;  \
            if (BS_OK != TXT_Atoui(_pszLen, &_ulParamLen)){break;}   \
            _pszStart[_ulParamLen] = '\0';  _pucParamdata = _pszStart; \
            _pszStart += (_ulParamLen + 1); \
            {
            
#define _SCALL_SCAN_PARAM_LINE_END()    } } }while(0)

typedef enum
{
    SCALL_PARAM_TYPE_STRING = 0,
    SCALL_PARAM_TYPE_INT,
    SCALL_PAARM_TYPE_BOOL,

    SCALL_PARAM_TYPE_INVALID
}_SCALL_PARAM_TYPE_E;

typedef struct
{
    _SCALL_PARAM_TYPE_E eType;  
    CHAR *pszType;              
}_SCALL_PARAM_TYPE_MAP_S;

typedef struct
{
    _SCALL_PARAM_TYPE_E eParamType;
    UINT ulParamLen;
    CHAR *pcParamData;
}_SCALL_PARAM_S;

typedef struct
{
    CHAR *pszFuncName;
    UINT ulParamNum;
    _SCALL_PARAM_S astParam[_SCALL_MAX_PARAM_NUM];
}_SCALL_MSG_S;

typedef struct
{
    UINT ulSsltcpId;
    MBUF_S *pstReadMbuf;
    UINT  ulTotalReadLen; 
    MBUF_S *pstWriteMbuf;
}_SCALL_STATE_S;

static _SCALL_PARAM_TYPE_MAP_S g_astSCallParamTypeMap[] = 
{
    {SCALL_PARAM_TYPE_STRING, "string"},
    {SCALL_PARAM_TYPE_INT, "integer"},
    {SCALL_PAARM_TYPE_BOOL, "boolean"}

};

static UINT g_ulSCallServerTid = 0;
static USHORT g_usSCallServerPort = _SCALL_DFT_SERVER_PORT;
static MSGQUE_HANDLE g_hSCallQueId = NULL;
static EVENT_HANDLE g_hSCallEventId = 0;
static SEM_HANDLE g_hSCallSem = 0;
static UINT g_uiScallDbgFlag = 0;

static _SCALL_PARAM_TYPE_E _SCALL_GetTypeByTypeName(IN CHAR *pszTypeName)
{
    UINT i;
    
    BS_DBGASSERT(NULL != pszTypeName);
    
    for (i=0; i<sizeof(g_astSCallParamTypeMap) / sizeof(_SCALL_PARAM_TYPE_MAP_S); i++)
    {
        if (stricmp(pszTypeName, g_astSCallParamTypeMap[i].pszType) == 0)
        {
            return g_astSCallParamTypeMap[i].eType;
        }
    }

    return SCALL_PARAM_TYPE_INVALID;
}

static _SCALL_STATE_S * _SCALL_MallocState()
{
    _SCALL_STATE_S *pstState;

    pstState = MEM_ZMalloc(sizeof(_SCALL_STATE_S));
    if (NULL == pstState)
    {
        return NULL;
    }

    return pstState;
}

static VOID _SCALL_FreeState(IN _SCALL_STATE_S *pstState)
{
    if (NULL != pstState->pstReadMbuf)
    {
        MBUF_Free(pstState->pstReadMbuf);
    }

    if (NULL != pstState->pstWriteMbuf)
    {
        MBUF_Free(pstState->pstWriteMbuf);
    }

    MEM_Free(pstState);
}

static VOID _SCALL_CloseConnection(IN UINT ulSsltcpId)
{
    USER_HANDLE_S *pstUserHandle;
    _SCALL_STATE_S *pstState;
    
    if (! SSLTCP_IsValid(ulSsltcpId))
    {
        return;
    }

    SSLTCP_GetAsyn(ulSsltcpId, &pstUserHandle);
    pstState = pstUserHandle->ahUserHandle[0];

    SSLTCP_Close(ulSsltcpId);
    _SCALL_FreeState(pstState);
}

static BOOL_T _SCALL_IsReadMsgFinish(IN _SCALL_STATE_S *pstState)
{
    INT iOffset;
    CHAR szTotalLenString[32];
    
    
    if (pstState->ulTotalReadLen == 0)
    {
        iOffset = MBUF_FindByte(pstState->pstReadMbuf, 0, _SCALL_LINE_END_CHAR);
        if (iOffset < 0)
        {
            return FALSE;
        }
        if (iOffset >= sizeof(szTotalLenString))
        {
            BS_DBGASSERT(0);
            return FALSE;
        }
        
        MBUF_CopyFromMbufToBuf(pstState->pstReadMbuf, 0, iOffset, szTotalLenString);
        szTotalLenString[iOffset] = '\0';

        TXT_Atoui(szTotalLenString, &pstState->ulTotalReadLen);
        pstState->ulTotalReadLen += (iOffset + 1);    
    }

    if (pstState->ulTotalReadLen <= MBUF_TOTAL_DATA_LEN(pstState->pstReadMbuf))
    {
        return TRUE;
    }

    return FALSE;
}


static BS_STATUS _SCALL_ParseMsg(IN UCHAR *pucRecvMsg, OUT _SCALL_MSG_S *pstMsg)
{
    CHAR *pcSplit;
    CHAR *pcStart;
    _SCALL_PARAM_TYPE_E eType;
    UINT ulLen;
    CHAR *pszParamData;
    UINT i;

    
    pcStart = strchr(pucRecvMsg, _SCALL_LINE_END_CHAR);
    pcStart++;
    
    
    pcSplit = strchr(pcStart, _SCALL_LINE_END_CHAR);
    if (NULL == pcSplit)
    {
        RETURN(BS_BAD_REQUEST);
    }

    pstMsg->pszFuncName = pcStart;
    *pcSplit = '\0';
    pcStart = pcSplit + 1;

    
    
    i = 0;
    _SCALL_SCAN_PARAM_LINE_BEGIN(pcStart, eType, ulLen, pszParamData)
    {
        if (i >= _SCALL_MAX_PARAM_NUM)
        {
            RETURN(BS_BAD_REQUEST);
        }
        pstMsg->astParam[i].eParamType = eType;
        pstMsg->astParam[i].ulParamLen = ulLen;
        pstMsg->astParam[i].pcParamData = pszParamData;
        i++;
    }_SCALL_SCAN_PARAM_LINE_END();

    pstMsg->ulParamNum = i;

    return BS_OK;
}

static MBUF_S * _SCALL_FormSendData
(
    IN UINT ulRetType,
    IN UINT64 ret,
    IN CHAR *pszParamFmt,
    IN UINT64 *params
)
{
    MBUF_S *pstMbuf;
    CHAR szNummber[16];
    CHAR szLength[16];
    CHAR szTmp[128];
    UINT ulParamNum = strlen(pszParamFmt);
    UINT i;
    UINT ulParam;
    UINT *pulParam;
    UINT uiLen;

    
    pstMbuf = MBUF_CreateByCopyBuf(10, "0;0;\n", strlen("0;0;\n"), MBUF_DATA_DATA);
    if (NULL == pstMbuf)
    {
        return NULL;
    }

    switch (ulRetType)
    {
        case FUNCTBL_RET_VOID:
            MBUF_CopyFromBufToMbuf(pstMbuf, MBUF_TOTAL_DATA_LEN(pstMbuf),
                "void;0;\n", STR_LEN("void;0;\n"));
            break;
        case FUNCTBL_RET_UINT32:
            sprintf(szNummber, "%lu", HANDLE_UINT(ret));
            sprintf(szLength, "%lu;", strlen(szNummber));
            TXT_Strlcpy(szTmp, "integer;", sizeof(szTmp));
            TXT_Strlcat(szTmp, szLength, sizeof(szTmp));
            TXT_Strlcat(szTmp, szNummber, sizeof(szTmp));
            TXT_Strlcat(szTmp, "\n", sizeof(szTmp));
            MBUF_CopyFromBufToMbuf(pstMbuf, MBUF_TOTAL_DATA_LEN(pstMbuf),
                szTmp, strlen(szTmp));
            break;
        case FUNCTBL_RET_BOOL:
            sprintf(szNummber, "%d\n", (HANDLE_UINT(ret) == 0 ? 0 : 1));
            TXT_Strlcpy(szTmp, "boolean;1;", sizeof(szTmp));
            TXT_Strlcat(szTmp, szNummber, sizeof(szTmp));
            MBUF_CopyFromBufToMbuf(pstMbuf, MBUF_TOTAL_DATA_LEN(pstMbuf),
                szTmp, strlen(szTmp));
            break;

         case FUNCTBL_RET_STRING:
            if (NULL != ret)
            {
                sprintf(szLength, "%lu;", STRING_GetLength(ret));
                TXT_Strlcpy(szTmp, "string;", sizeof(szTmp));
                TXT_Strlcat(szTmp, szLength, sizeof(szTmp));
                MBUF_CopyFromBufToMbuf(pstMbuf, MBUF_TOTAL_DATA_LEN(pstMbuf),
                    szTmp, strlen(szTmp));
                MBUF_CopyFromBufToMbuf(pstMbuf, MBUF_TOTAL_DATA_LEN(pstMbuf),
                    STRING_GetBuf(ret), STRING_GetLength(ret));
                STRING_Delete(ret);
                MBUF_CopyFromBufToMbuf(pstMbuf, MBUF_TOTAL_DATA_LEN(pstMbuf),
                    "\n", STR_LEN("\n"));
                break;
            }
            

        case FUNCTBL_RET_SEQUENCE:
            if (NULL == ret)
            {
                ret = "";
            }
            sprintf(szLength, "%lu;", strlen(ret));
            TXT_Strlcpy(szTmp, "string;", sizeof(szTmp));
            TXT_Strlcat(szTmp, szLength, sizeof(szTmp));
            MBUF_CopyFromBufToMbuf(pstMbuf, MBUF_TOTAL_DATA_LEN(pstMbuf),
                szTmp, strlen(szTmp));
            MBUF_CopyFromBufToMbuf(pstMbuf, MBUF_TOTAL_DATA_LEN(pstMbuf),
                ret, strlen(ret));
            MBUF_CopyFromBufToMbuf(pstMbuf, MBUF_TOTAL_DATA_LEN(pstMbuf),
                "\n", STR_LEN("\n"));
            break;

        default:
            BS_DBGASSERT(0);
            break;
    }

    for (i=0; i<ulParamNum; i++)
    {
        switch (pszParamFmt[i])
        {
            
            case 'p':
            {
                pulParam = (UINT *)(params[i]);
                ulParam = *pulParam;
                sprintf(szNummber, "%lu", ulParam);
                sprintf(szLength, "%lu;", strlen(szNummber));
                TXT_Strlcpy(szTmp, "integer;", sizeof(szTmp));
                TXT_Strlcat(szTmp, szLength, sizeof(szTmp));
                TXT_Strlcat(szTmp, szNummber, sizeof(szTmp));
                TXT_Strlcat(szTmp, "\n", sizeof(szTmp));
                MBUF_CopyFromBufToMbuf(pstMbuf, MBUF_TOTAL_DATA_LEN(pstMbuf),
                    szTmp, strlen(szTmp));
                break;
            }
            
            case 'o':
            {
                sprintf(szLength, "%lu;", STRING_GetLength(params[i]));
                TXT_Strlcpy(szTmp, "string;", sizeof(szTmp));
                TXT_Strlcat(szTmp, szLength, sizeof(szTmp));
                MBUF_CopyFromBufToMbuf(pstMbuf, MBUF_TOTAL_DATA_LEN(pstMbuf),
                    szTmp, strlen(szTmp));
                MBUF_CopyFromBufToMbuf(pstMbuf, MBUF_TOTAL_DATA_LEN(pstMbuf),
                    STRING_GetBuf(params[i]), STRING_GetLength(params[i]));
                MBUF_CopyFromBufToMbuf(pstMbuf, MBUF_TOTAL_DATA_LEN(pstMbuf),
                    "\n", STR_LEN("\n"));
                break;
            }
            
            default:
            {
                MBUF_CopyFromBufToMbuf(pstMbuf, MBUF_TOTAL_DATA_LEN(pstMbuf),
                    "void;0;\n", STR_LEN("void;0;\n"));
                break;
            }
        }
    }

    sprintf(szLength, "%d\n", MBUF_TOTAL_DATA_LEN(pstMbuf));

    uiLen = strlen(szLength);
    MBUF_Prepend(pstMbuf, uiLen);
    MBUF_CopyFromBufToMbuf(pstMbuf, 0, szLength, uiLen);

    return pstMbuf;    
}

static VOID _SCALL_OutPutErrInfo(IN _SCALL_STATE_S *pstState, IN CHAR *pszErrInfo)
{
    CHAR szLength[16];
    CHAR szErrCode[32];
    UINT ulWriteSize;
    UINT ulErrInfoLen = strlen(pszErrInfo);

    sprintf(szErrCode, "1;%lu;", ulErrInfoLen);
    sprintf(szLength, "%lu\n%s", strlen(szErrCode) + ulErrInfoLen + 1, szErrCode);
    
    SSLTCP_Write(pstState->ulSsltcpId, szLength, strlen(szLength), &ulWriteSize);
    SSLTCP_Write(pstState->ulSsltcpId, pszErrInfo, strlen(pszErrInfo), &ulWriteSize);
    SSLTCP_Write(pstState->ulSsltcpId, "\n", 1, &ulWriteSize);
}

static BS_STATUS scall_WriteMbuf(IN UINT ulSslTcpId, IN MBUF_S *pstMbuf, OUT UINT *pulWriteLen)
{
    UCHAR *pucData;
    UINT ulLen;
    UINT ulSendLenTmp;
    UINT ulSendLen = 0;

    if (NULL == pstMbuf)
    {
        RETURN(BS_NULL_PARA);
    }

    *pulWriteLen = 0;

    if (MBUF_TOTAL_DATA_LEN(pstMbuf) == 0)
    {
        return BS_OK;
    }

    MBUF_SCAN_DATABLOCK_BEGIN(pstMbuf, pucData, ulLen)
    {
        if (BS_OK != SSLTCP_Write(ulSslTcpId, pucData, ulLen, &ulSendLenTmp))
        {
            RETURN(BS_ERR);
        }
        ulSendLen += ulSendLenTmp;
        if (ulSendLenTmp < ulLen)
        {
            *pulWriteLen = ulSendLen;
            return BS_OK;
        }
    }MBUF_SCAN_END();

    *pulWriteLen = ulSendLen;

    return BS_OK;
}

static BS_STATUS _SCALL_ProcReadMsg(IN _SCALL_STATE_S *pstState, IN UCHAR *pucRecvMsg)
{
    BS_STATUS eRet;
    _SCALL_MSG_S stMsg;
    PF_FUNCTBL_FUNC_X pfFunc;
    UINT ulRetType;
    CHAR szFuncFmt[128];
    U64 params[_SCALL_MAX_PARAM_NUM];
    UINT aulIntParams[_SCALL_MAX_PARAM_NUM];
    HSTRING ahStringParams[_SCALL_MAX_PARAM_NUM];
    UINT ulFmtLen;
    UINT i;
    UINT64 ret;
    MBUF_S *pstSendMbuf;
    UINT ulSendLen;
    CHAR szErrInfo[512] = "";

    Mem_Zero(ahStringParams, sizeof(ahStringParams));

    eRet = _SCALL_ParseMsg(pucRecvMsg, &stMsg);
    if (BS_OK != eRet)
    {
        return eRet;
    }

    
    pfFunc = FUNCTBL_GetFunc(stMsg.pszFuncName, &ulRetType, szFuncFmt);
    if (NULL == pfFunc)
    {
        snprintf(szErrInfo, sizeof(szErrInfo)-1, "Can't find function %s", stMsg.pszFuncName);
        _SCALL_OutPutErrInfo(pstState, szErrInfo);
        RETURN(BS_ERR);
    }
    
    ulFmtLen = strlen(szFuncFmt);
    for (i=0; i<ulFmtLen; i++)
    {
        switch (szFuncFmt[i])
        {
            case 'u':
            {
                TXT_Atoui(stMsg.astParam[i].pcParamData, &aulIntParams[i]);
                params[i] = UINT_HANDLE(aulIntParams[i]);
                break;
            }
            case 'p':
            {
                TXT_Atoui(stMsg.astParam[i].pcParamData, &aulIntParams[i]);
                params[i] = &aulIntParams[i];
                break;
            }
            case 'b':
            {
                params[i] = stMsg.astParam[i].pcParamData;
                break;
            }
            case 's':
            case 'o':
            {
                if (NULL == (ahStringParams[i] = STRING_Create()))
                {
                    RETURN(BS_NO_MEMORY);
                }
                if (BS_OK != STRING_CpyFromBuf(ahStringParams[i], stMsg.astParam[i].pcParamData))
                {
                    RETURN(BS_NO_MEMORY);
                }
                params[i] = ahStringParams[i];
                break;
            }

            default:
                BS_DBGASSERT(0);
                break;            
        }
    }

    
    ret = pfFunc (params[0], params[1], params[2], params[3], params[4],
                    params[5], params[6], param[7], params[8], params[9]);

    pstSendMbuf = _SCALL_FormSendData(ulRetType, ret, szFuncFmt, params);
    for (i=0; i<_SCALL_MAX_PARAM_NUM; i++)
    {
        if (ahStringParams[i] != NULL)
        {
            STRING_Delete(ahStringParams[i]);
        }
    }
    
    if (NULL == pstSendMbuf)
    {
        RETURN(BS_ERR);
    }

    if (BS_OK != scall_WriteMbuf(pstState->ulSsltcpId, pstSendMbuf, &ulSendLen))
    {
        MBUF_Free(pstSendMbuf);
        RETURN(BS_ERR);
    }
    if (ulSendLen == MBUF_TOTAL_DATA_LEN(pstSendMbuf))
    {
        MBUF_Free(pstSendMbuf);
        pstState->pstWriteMbuf = NULL;
        RETURN(BS_STOP);
    }
    else
    {
        MBUF_CutHead(pstSendMbuf, ulSendLen);
        pstState->pstWriteMbuf = pstSendMbuf;
    }

    return BS_OK;
}

static BS_STATUS _SCALL_ProcReadData(IN _SCALL_STATE_S *pstState)
{

    UCHAR *pucRecvMsg;
    HANDLE hMemHandle;
    BS_STATUS eRet;

    pucRecvMsg = MBUF_GetContinueMem(pstState->pstReadMbuf, 0,
        MBUF_TOTAL_DATA_LEN(pstState->pstReadMbuf), &hMemHandle);
    if (NULL == pucRecvMsg)
    {
        RETURN(BS_ERR);
    }

    eRet = _SCALL_ProcReadMsg(pstState, pucRecvMsg);

    MBUF_FreeContinueMem(hMemHandle);
    MBUF_Free(pstState->pstReadMbuf);
    pstState->pstReadMbuf = NULL;
    
    return eRet;
}

static BS_STATUS _SCALL_ProcEvent(IN UINT ulSsltcpId, IN UINT ulEvent)
{
    USER_HANDLE_S *pstUserHandle;
    _SCALL_STATE_S *pstState;
    UCHAR aucData[1024];
    UINT ulLen;
    UCHAR *pucData;
    UINT ulSendLen;
    UINT ulTotalSendLen = 0;
    BS_STATUS eRet;

    if (TRUE != SSLTCP_IsValid(ulSsltcpId))
    {
        RETURN(BS_NO_SUCH);
    }

    SSLTCP_GetAsyn(ulSsltcpId, &pstUserHandle);
    pstState = pstUserHandle->ahUserHandle[0];

    if (ulEvent & SSLTCP_EVENT_WRITE)
    {
        if ((pstState->pstWriteMbuf != NULL) && (MBUF_TOTAL_DATA_LEN(pstState->pstWriteMbuf) > 0))
        {
            MBUF_SCAN_DATABLOCK_BEGIN(pstState->pstWriteMbuf, pucData, ulLen)
            {
                if (BS_OK != SSLTCP_Write(ulSsltcpId, pucData, ulLen, &ulSendLen))
                {
                    _SCALL_CloseConnection(ulSsltcpId);
                    RETURN(BS_ERR);
                }
                ulTotalSendLen += ulSendLen;

                if (ulSendLen < ulLen)
                {
                    break;
                }
        
            }MBUF_SCAN_END();

            MBUF_CutHead(pstState->pstWriteMbuf, ulSendLen);
            if (MBUF_TOTAL_DATA_LEN(pstState->pstWriteMbuf) == 0)
            {
                MBUF_Free(pstState->pstWriteMbuf);
                pstState->pstWriteMbuf = NULL;
                _SCALL_CloseConnection(pstState->ulSsltcpId);
            }
        }
    }

    if (ulEvent & SSLTCP_EVENT_READ)
    {
        if (BS_OK != SSLTCP_Read(ulSsltcpId, aucData, sizeof(aucData), &ulLen))
        {
            _SCALL_CloseConnection(ulSsltcpId);
            RETURN(BS_ERR);
        }

        if (ulLen > 0)
        {
            if (pstState->pstReadMbuf == NULL)
            {
                pstState->pstReadMbuf = MBUF_CreateByCopyBuf(0, aucData, ulLen, 0);
                if (NULL == pstState->pstReadMbuf)
                {
                    _SCALL_CloseConnection(ulSsltcpId);
                    RETURN(BS_NO_MEMORY);
                }
            }
            else
            {
                if (BS_OK != MBUF_CopyFromBufToMbuf(pstState->pstReadMbuf, MBUF_TOTAL_DATA_LEN(pstState->pstReadMbuf),
                    aucData, ulLen))
                {
                    _SCALL_CloseConnection(ulSsltcpId);
                    RETURN(BS_NO_MEMORY);
                }
            }
        }

        
        if (TRUE == _SCALL_IsReadMsgFinish(pstState))
        {
            if (BS_OK != (eRet = _SCALL_ProcReadData(pstState)))
            {
                _SCALL_CloseConnection(ulSsltcpId);
                return eRet;
            }
        }        
    }
    
    return BS_OK;
}

static BS_STATUS _SCALL_Main()
{
    UINT uiEvent;
    MSGQUE_MSG_S stMsg;
    UINT ulSslTcpId;
    UINT ulSslTcpEvent;
    UINT uiMsgType;
    
    while (BS_OK == Event_Read(g_hSCallEventId, _SCALL_EVNET, &uiEvent, BS_WAIT, BS_WAIT_FOREVER))
    {
        if (uiEvent & _SCALL_EVNET)
        {
            while (BS_OK == MSGQUE_ReadMsg(g_hSCallQueId, &stMsg))
            {
                uiMsgType = HANDLE_UINT(stMsg.ahMsg[0]);
                ulSslTcpId = HANDLE_UINT(stMsg.ahMsg[1]);
                ulSslTcpEvent = HANDLE_UINT(stMsg.ahMsg[2]);

                SEM_P(g_hSCallSem, BS_WAIT, BS_WAIT_FOREVER);
                if (_SCALL_MSG_TYPE_TIMEOUT == uiMsgType)
                {
                    _SCALL_DBG_PRINT(_SCALL_DBG_FLAG_PROCESS, ("\r\n Scall connection time out."));
                    _SCALL_CloseConnection(ulSslTcpId);
                }
                else if (_SCALL_MSG_TYPE_SSLTCP == uiMsgType)
                {
                    _SCALL_ProcEvent(ulSslTcpId, ulSslTcpEvent);
                }
                else
                {
                    BS_DBGASSERT(0);
                }
                SEM_V(g_hSCallSem);
            }
        }
    }

	return BS_OK;
}

static BS_STATUS _SCALL_ProcessSsltcpEvent(IN UINT ulSslTcpId, IN UINT ulEvent, IN USER_HANDLE_S *pstUserHandle)
{
    MSGQUE_MSG_S stMsg;

    _SCALL_DBG_PRINT(_SCALL_DBG_FLAG_PROCESS, ("\r\n Scall receive event 0x%x", ulEvent));

    stMsg.ahMsg[0] = UINT_HANDLE(_SCALL_MSG_TYPE_SSLTCP);
    stMsg.ahMsg[1] = UINT_HANDLE(ulSslTcpId);
    stMsg.ahMsg[2] = UINT_HANDLE(ulEvent);

    if (BS_OK != MSGQUE_WriteMsg(g_hSCallQueId, &stMsg))
    {
        
        SEM_P(g_hSCallSem, BS_WAIT, BS_WAIT_FOREVER);
        _SCALL_CloseConnection(ulSslTcpId);
        SEM_V(g_hSCallSem);
        return BS_OK;
    }

    (VOID) Event_Write(g_hSCallEventId, _SCALL_EVNET);

    return BS_OK;
}

static VOID _SCALL_TimerOut(IN HANDLE hTimerId, IN USER_HANDLE_S *pstUserHandle)
{
    UINT ulSslTcpId;
    MSGQUE_MSG_S stMsg;
    MTIMER_S *timer;
    
    ulSslTcpId = HANDLE_UINT(pstUserHandle->ahUserHandle[0]);
    timer = pstUserHandle->ahUserHandle[1];

    stMsg.ahMsg[0] = UINT_HANDLE(_SCALL_MSG_TYPE_TIMEOUT);
    stMsg.ahMsg[1] = UINT_HANDLE(ulSslTcpId);

    if (BS_OK != MSGQUE_WriteMsg(g_hSCallQueId, &stMsg))
    {
        return;
    }

    (VOID) Event_Write(g_hSCallEventId, _SCALL_EVNET);

    MTimer_Del(timer);

    return;
}

static BS_STATUS _SCALL_ProcessAccept(IN UINT ulSslTcpId, IN UINT ulEvent, IN USER_HANDLE_S *pstUserHandle)
{
    UINT ulAcceptId;
    USER_HANDLE_S stHandle;
    _SCALL_STATE_S *pstState;
    USER_HANDLE_S stUserHandle;
    MTIMER_S *timer;
    
    _SCALL_DBG_PRINT(_SCALL_DBG_FLAG_PROCESS, ("\r\n Scall receive listen socket event 0x%x", ulEvent));

    if (ulEvent & SSLTCP_EVENT_READ)
    {
        if (BS_OK ==SSLTCP_Accept(ulSslTcpId, &ulAcceptId))
        {
            _SCALL_DBG_PRINT(_SCALL_DBG_FLAG_PROCESS, ("\r\n Scall accept a new connection"));

            pstState = _SCALL_MallocState();
            if (NULL == pstState)
            {
                SSLTCP_Close(ulAcceptId);
                RETURN(BS_ERR);
            }
            pstState->ulSsltcpId = ulAcceptId;
            stHandle.ahUserHandle[0] = pstState;
            timer = MEM_ZMalloc(sizeof(MTIMER_S));
            if (timer == NULL) {
                SSLTCP_Close(ulAcceptId);
                RETURN(BS_ERR);
            }
            stUserHandle.ahUserHandle[0] = UINT_HANDLE(ulAcceptId);
            stUserHandle.ahUserHandle[1] = timer;
            MTimer_Add(timer, 5000, TIMER_FLAG_CYCLE,
                    _SCALL_TimerOut, &stUserHandle);
            SSLTCP_SetAsyn(NULL, ulAcceptId, SSLTCP_EVENT_ALL,
                    _SCALL_ProcessSsltcpEvent, &stHandle);
        }
    }

    return BS_OK;
}

static BS_STATUS scall_Init()
{
    g_hSCallSem = SEM_CCreate("SCall", 1);
    if (g_hSCallSem == 0)
    {
        RETURN(BS_ERR);
    }
    
    g_hSCallEventId = Event_Create();
    if (g_hSCallEventId == NULL)
    {
        SEM_Destory(g_hSCallSem);
        RETURN(BS_ERR);
    }

    g_hSCallQueId = MSGQUE_Create(512);
    if (NULL == g_hSCallQueId)
    {
        Event_Delete(g_hSCallEventId);
        g_hSCallEventId = 0;
        SEM_Destory(g_hSCallSem);
        g_hSCallSem = 0;
        RETURN(BS_ERR);
    }

    return BS_OK;
}

static BS_STATUS _SCALL_Thread(IN USER_HANDLE_S *pstUserHandle)
{
    CFF_HANDLE hCff;
    CHAR szIniFile[FILE_MAX_PATH_LEN + 1];
    UINT ulPort = 0;
    UINT ulSslTcpId;
    BS_STATUS eRet;
    USER_HANDLE_S stUserHandle;

    eRet = scall_Init();
    if (BS_OK != eRet)
    {
        return eRet;
    }

    if ((BS_OK ==  LOCAL_INFO_ExpandToConfPath(_SCALL_DFT_INI_FILE, szIniFile))
        && (NULL != (hCff = CFF_INI_Open(szIniFile, CFF_FLAG_CREATE_IF_NOT_EXIST | CFF_FLAG_READ_ONLY))))
    {
        if ((BS_OK == CFF_GetPropAsUint(hCff, "service", "port", &ulPort)) && (ulPort != 0))
        {
            g_usSCallServerPort = (USHORT)ulPort;
        }

        CFF_Close(hCff);
    }

    
    ulSslTcpId = SSLTCP_Create("tcp", AF_INET, NULL);
    if (0 == ulSslTcpId)
    {
        RETURN(BS_CAN_NOT_OPEN);
    }

    eRet = SSLTCP_Listen(ulSslTcpId, 0, g_usSCallServerPort, 5);
    if (BS_OK != eRet)
    {
        return eRet;
    }

    (VOID) SSLTCP_SetAsyn(NULL, ulSslTcpId, SSLTCP_EVENT_ALL, _SCALL_ProcessAccept, &stUserHandle);

    _SCALL_Main();

    SSLTCP_Close(ulSslTcpId);
    return BS_OK;
}

BS_STATUS SCALL_Start()
{
    
    g_ulSCallServerTid = THREAD_Create("SCallServer", NULL, _SCALL_Thread, NULL);
    if (THREAD_ID_INVALID == g_ulSCallServerTid) {
        RETURN(BS_ERR);
    }

    return BS_OK;
}

PLUG_API BS_STATUS SCALL_DebugProcess(IN UINT uiAgrc, IN CHAR **ppArgv)
{
    if (strcmp(ppArgv[0], "no") == 0)
    {
        g_uiScallDbgFlag &= ~((UINT)_SCALL_DBG_FLAG_PROCESS);
    }
    else
    {
        g_uiScallDbgFlag |= _SCALL_DBG_FLAG_PROCESS;
    }

    return BS_OK;
}


