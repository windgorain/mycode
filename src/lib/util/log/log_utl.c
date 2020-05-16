/*================================================================
*   Created by LiXingang
*   Description:
*
================================================================*/
#include "bs.h"
#include "utl/txt_utl.h"
#include "utl/buffer_utl.h"
#include "utl/lstr_utl.h"
#include "utl/file_utl.h"
#include "utl/data2hex_utl.h"
#include "utl/log_utl.h"

static inline void _log_Print(char *str, int len)
{
    char info[1024];
    char *tmp = str;
    int left_len = len;
    int print_len;

    while (left_len > 0) {
        print_len = MIN(left_len, sizeof(info) -1);

        memcpy(info, tmp, print_len);
        info[print_len] = '\0';

        printf("%s", info);

        left_len -= print_len;
        tmp += print_len;
    }
}

static inline void _log_WriteFile(FILE *fp, char *str, int len)
{
    fwrite(str, 1, len, fp);
}

static void _log_Output(void *pData, UINT uiLen, USER_HANDLE_S *ud)
{
    LOG_UTL_S *pstLogCtrl = ud->ahUserHandle[0];

    if (pstLogCtrl->log_file_fp == NULL) {
        pstLogCtrl->log_file_fp = fopen(pstLogCtrl->log_file_name, "a+");
    }

    if (pstLogCtrl->log_file_fp != NULL) {
        _log_WriteFile(pstLogCtrl->log_file_fp, pData, uiLen);
        fflush(pstLogCtrl->log_file_fp);
    }

    if (pstLogCtrl->print) {
        _log_Print(pData, uiLen);
    }
}

static int _log_process_kv(IN LSTR_S *pstKey, IN LSTR_S *pstValue, IN void *pUserhandle)
{
    LOG_UTL_S *pstLogCtrl = pUserhandle;
    char buf[256];

    if ((LSTR_StrCmp(pstKey, "filename") == 0) && (pstValue->uiLen > 0)) {
        LSTR_Strlcpy(pstValue, sizeof(buf), buf);
        LOG_SetFileName(pstLogCtrl, buf);
        pstLogCtrl->file = 1;
    } else if ((LSTR_StrCmp(pstKey, "count_file") == 0) && (pstValue->uiLen > 0)) {
        LSTR_Strlcpy(pstValue, sizeof(buf), buf);
        LOG_SetCountFile(pstLogCtrl, buf);
    } else if ((LSTR_StrCmp(pstKey, "print") == 0) && (pstValue->uiLen > 0)) {
        if (pstValue->pcData[0] == '1') {
            pstLogCtrl->print = 1;
        }
    } else if ((LSTR_StrCmp(pstKey, "syslog") == 0) && (pstValue->uiLen > 0)) {
         if (pstValue->pcData[0] == '1') {
            pstLogCtrl->syslog = 1;
        }
    }

    return 0;
}

int LOG_Init(INOUT LOG_UTL_S *pstLogCtrl)
{
    USER_HANDLE_S ud;

    memset(pstLogCtrl, 0, sizeof(LOG_UTL_S));

    ud.ahUserHandle[0] = pstLogCtrl;
    pstLogCtrl->hBuffer = BUFFER_Create(1024, _log_Output, &ud);
    if (NULL == pstLogCtrl) {
        RETURN(BS_NO_MEMORY);
    }

    return 0;
}

void LOG_Fini(INOUT LOG_UTL_S *pstLogCtrl)
{
    if (pstLogCtrl->log_file_name) {
        MEM_Free(pstLogCtrl->log_file_name);
        pstLogCtrl->log_file_name = NULL;
    }
    if (pstLogCtrl->log_file_fp) {
        FILE_Close(pstLogCtrl->log_file_fp);
        pstLogCtrl->log_file_fp = NULL;
    }
    if (pstLogCtrl->log_count_file) {
        MEM_Free(pstLogCtrl->log_count_file);
        pstLogCtrl->log_count_file = NULL;
    }
    if (pstLogCtrl->hBuffer) {
        BUFFER_Destory(pstLogCtrl->hBuffer);
        pstLogCtrl->hBuffer = NULL;
    }

    memset(pstLogCtrl, 0, sizeof(LOG_UTL_S));
}

/* {filename:xxx;print:1;} */
int LOG_ParseConfig(LOG_UTL_S *pstLogCtrl, char *pcConfigString,
        UINT uiConfigStringLen)
{
    char *start, *end;
    LSTR_S stConfig;

    if (0 != TXT_FindBracket(pcConfigString, uiConfigStringLen,
                "{}", &start, &end)) {
        return -1;
    }

    start ++;

    stConfig.pcData = start;
    stConfig.uiLen = end - start;

    LSTR_ScanMultiKV(&stConfig, ';', ':', _log_process_kv, pstLogCtrl);

    return 0;
}

void LOG_SetBaseDir(IN LOG_UTL_S *pstLogCtrl, IN char *pcBaseDir)
{
    pstLogCtrl->base_dir = pcBaseDir;
}

void LOG_SetFileName(IN LOG_UTL_S *pstLogCtrl, IN char *pcFileName)
{
    pstLogCtrl->log_file_name =
        FILE_Dup2AbsPath(pstLogCtrl->base_dir, pcFileName);
}

void LOG_SetCountFile(IN LOG_UTL_S *pstLogCtrl, IN char *pcFileName)
{
    pstLogCtrl->log_count_file =
        FILE_Dup2AbsPath(pstLogCtrl->base_dir, pcFileName);
}

void LOG_SetPrint(IN LOG_UTL_S *pstLogCtrl, IN BOOL_T bPrint)
{
    if (bPrint) {
        pstLogCtrl->print = TRUE;
    } else {
        pstLogCtrl->print = FALSE;
    }
}

char * LOG_GetTimeStringBySecond(time_t seconds)
{
	struct tm  *timenow;
    char *time_str;
    int len;

	timenow = localtime(&seconds);

    time_str = asctime(timenow);
    len = strlen(time_str);
    time_str[len - 1] = '\0';

    return time_str;
}

char * LOG_GetNowTimeString()
{
    time_t now;

	time(&now);

    return LOG_GetTimeStringBySecond(now);
}

static void _log_WriteCount(LOG_UTL_S *pstLogCtrl)
{
    FILE *fp;

    if (pstLogCtrl->log_count_file == NULL) {
        return;
    }

    fp = fopen(pstLogCtrl->log_count_file, "a+");
    if (fp == NULL) {
        return;
    }

    fprintf(fp, "{\"time\":\"%s\",\"count\":%llu}\n",
            LOG_GetTimeStringBySecond(time(0)), pstLogCtrl->log_count);

    pstLogCtrl->log_count = 0;

    fclose(fp);
}

void LOG_AddCount(LOG_UTL_S *pstLogCtrl)
{
    pstLogCtrl->log_count ++;

    if ((pstLogCtrl->log_count & 0xffff) == 0xffff) {
        _log_WriteCount(pstLogCtrl);
    }
}

void LOG_Output(IN LOG_UTL_S *pstLogCtrl, char *str, int len)
{
    BUFFER_Write(pstLogCtrl->hBuffer, str, len);
}

void LOG_OutputLn(IN LOG_UTL_S *pstLogCtrl, char *str, int len)
{
    if (len > 0) {
        LOG_Output(pstLogCtrl, str, len);
    }
    LOG_Output(pstLogCtrl, "\n", 1);
}

void LOG_OutputString(IN LOG_UTL_S *pstLogCtrl, char *str)
{
    LOG_Output(pstLogCtrl, str, strlen(str));
}

void LOG_OutputStringLn(IN LOG_UTL_S *pstLogCtrl, char *str)
{
    LOG_OutputLn(pstLogCtrl, str, strlen(str));
}

void LOG_OutputByHex(IN LOG_UTL_S *pstLogCtrl, IN VOID *data, IN int len)
{
    char hex[1024 + 1];
    UCHAR *tmp = data;
    int left_len = len;
    int hex_data_len;

    while (left_len > 0) {
        hex_data_len = MIN(left_len, sizeof(hex)/2);
        DH_Data2HexString(tmp, hex_data_len, hex);
        LOG_Output(pstLogCtrl, hex, hex_data_len *2);
        left_len -= hex_data_len;
        tmp += hex_data_len;
    }
}

void LOG_OutputKeyValue(LOG_UTL_S *pstLogCtrl, char *key, char *value)
{
    LOG_OutputArgs(pstLogCtrl, ",\"%s\":\"%s\"", key, value);
}

void LOG_OutputKeyNum(LOG_UTL_S *pstLogCtrl, char *key, int num)
{
    LOG_OutputArgs(pstLogCtrl, ",\"%s\":\"%d\"", key, num);
}

void LOG_OutputArgs(IN LOG_UTL_S *pstLogCtrl, IN char *fmt, ...)
{
    va_list args;
    char msg[2048];

    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    LOG_OutputString(pstLogCtrl, msg);
}

void LOG_OutputHead(LOG_UTL_S *pstLogCtrl, char *log_type, int ver)
{
    time_t seconds;
    char info[256];

    seconds = time(NULL);

    snprintf(info, sizeof(info),
            "{\"log_type\":\"%s\",\"log_ver\":%d,\"time\":\"%s\"",
            log_type, ver, LOG_GetTimeStringBySecond(seconds));

    LOG_OutputString(pstLogCtrl, info);
}

void LOG_OutputTail(LOG_UTL_S *pstLogCtrl)
{
    LOG_OutputString(pstLogCtrl, "}\n");
}

void LOG_Flush(LOG_UTL_S *pstLogCtrl)
{
    BUFFER_Flush(pstLogCtrl->hBuffer);
}

int Log_FillHead(char *log_type, int ver, char *buf, int buflen)
{
    time_t seconds;
    seconds = time(NULL);

    return snprintf(buf, buflen,
            "{\"log_type\":\"%s\",\"log_ver\":%d,\"time\":\"%s\"",
            log_type, ver, LOG_GetTimeStringBySecond(seconds));
}

int Log_FillTail(char *buf, int buflen)
{
    int i = 0;

    buf[i++] = '}';
    buf[i++] = '\n';

    return i;
}
