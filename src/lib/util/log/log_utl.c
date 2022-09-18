/*================================================================
*   Created by LiXingang
*   Description:
*
================================================================*/
#include <sys/types.h>
#include <sys/socket.h>
#include "bs.h"
#include "utl/txt_utl.h"
#include "utl/syslog_utl.h"
#include "utl/buffer_utl.h"
#include "utl/lstr_utl.h"
#include "utl/file_utl.h"
#include "utl/socket_utl.h"
#include "utl/data2hex_utl.h"
#include "utl/log_utl.h"

static inline void _log_Print(char *str, int len)
{
    char info[1024] = "";
    char *tmp = str;
    int left_len = len;
    int print_len;

    while (left_len > 0) {
        print_len = MIN(left_len, sizeof(info) -1);

#ifndef __COVERITY__ 
        memcpy(info, tmp, print_len);
#endif
        info[print_len] = '\0';

        printf("%s \n", info);

        left_len -= print_len;
        tmp += print_len;
    }
}

static inline void _log_WriteFile(FILE *fp, char *str, int len)
{
    fwrite(str, 1, len, fp);
    fwrite("\n", 1, 1, fp);
}

static BOOL_T _log_check_logfile_exceed(LOG_UTL_S *pstLogCtrl)
{
    UINT64 filesize;

    if (pstLogCtrl->log_file_size == 0) {
        return FALSE;
    }

    if (0 != FILE_GetSize(pstLogCtrl->log_file_name, &filesize)) {
        return FALSE;
    }

    if (filesize >= pstLogCtrl->log_file_size)
    {
        return TRUE;
    }

    return FALSE;
}

/*send fail exceed 10 times, stop send and write log to log_file*/
/*if file_name not null, then logs that be sent fail will be written into log_file;*/
/*if file_enable is 1, then all logs will be written into log_file*/
static void _log_send_fail_write_file(LOG_UTL_S *pstLogCtrl,void* pData, UINT uiLen){
    if (! pstLogCtrl->log_file_fp && pstLogCtrl->log_file_name) {
        pstLogCtrl->log_file_fp = FILE_Open(pstLogCtrl->log_file_name, 1, "a+");
    }

    if (pstLogCtrl->log_file_fp) {
        if (_log_check_logfile_exceed(pstLogCtrl)) {
            fflush(pstLogCtrl->log_file_fp);
            ftruncate(fileno(pstLogCtrl->log_file_fp), 0);
            rewind(pstLogCtrl->log_file_fp);
        }
        _log_WriteFile(pstLogCtrl->log_file_fp, pData, uiLen);
    }
    return;
}

static void _log_Output(LOG_UTL_S *pstLogCtrl, void *pData, UINT uiLen)
{
    uint send_cout=0;

    if (pstLogCtrl->file) {
        if (! pstLogCtrl->log_file_fp && pstLogCtrl->log_file_name) {
            pstLogCtrl->log_file_fp = FILE_Open(pstLogCtrl->log_file_name, 1, "a+");
        }

        if (pstLogCtrl->log_file_fp) {
            if (_log_check_logfile_exceed(pstLogCtrl)) {
                fflush(pstLogCtrl->log_file_fp);
                ftruncate(fileno(pstLogCtrl->log_file_fp), 0);
                rewind(pstLogCtrl->log_file_fp);
            }
            _log_WriteFile(pstLogCtrl->log_file_fp, pData, uiLen);
            fflush(pstLogCtrl->log_file_fp);
        }
    }

    PF_LOG_OUT_FUNC output_func = pstLogCtrl->output_func;
    if (output_func) {
        output_func(pData, uiLen);
    }

    if (pstLogCtrl->syslog) {
        if (pstLogCtrl->syslog_msg_tag[0] && pstLogCtrl->syslog_opened == 0) {
            openlog(pstLogCtrl->syslog_msg_tag, LOG_PID, LOG_LOCAL6);
            pstLogCtrl->syslog_opened = 1;
        }
        syslog(LOG_LOCAL6, "%s", (char*)pData);
    }

    if (pstLogCtrl->print) {
        _log_Print(pData, uiLen);
    }
    
    if (pstLogCtrl->uds_file) {
        
        do 
        {
            if (pstLogCtrl->uds_fd <= 0) {
                pstLogCtrl->uds_fd = Socket_UnixClient(pstLogCtrl->uds_file, SOCK_DGRAM, 1);
            }

            if (pstLogCtrl->uds_fd>0) {
                int ret = Socket_Write(pstLogCtrl->uds_fd, pData, uiLen, 0);
                if (ret < 0) {
                    snprintf(pstLogCtrl->uds_last_err, sizeof(pstLogCtrl->uds_last_err),
                        "%s:%s", pstLogCtrl->uds_file, strerror(ret));
                    pstLogCtrl->uds_log_fail++;
                    send_cout++;
                
                    if (ret == ECONNREFUSED || ret == ENOTCONN || ret == ESHUTDOWN) {
                        Socket_Close(pstLogCtrl->uds_fd);
                        pstLogCtrl->uds_fd = 0;
                    }
                }
            } else{ 
                send_cout++;
            }
        } while(send_cout >=1 && send_cout < pstLogCtrl->send_fail_max_count);

        if(send_cout>=pstLogCtrl->send_fail_max_count){
            _log_send_fail_write_file(pstLogCtrl,pData,uiLen);
        }
    }

    pstLogCtrl->log_count++;

    return;
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
    } else if ((LSTR_StrCmp(pstKey, "uds_file") == 0) && (pstValue->uiLen > 0)) {
        LSTR_Strlcpy(pstValue, sizeof(buf), buf);
        LOG_SetUDSFile(pstLogCtrl, buf);
        pstLogCtrl->uds = 1;
    }

    return 0;
}

int LOG_Init(INOUT LOG_UTL_S *pstLogCtrl)
{
    memset(pstLogCtrl, 0, sizeof(LOG_UTL_S));
    return 0;
}

void LOG_Fini(INOUT LOG_UTL_S *pstLogCtrl)
{
    if (pstLogCtrl->log_file_name) {
        free(pstLogCtrl->log_file_name);
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
    if (pstLogCtrl->uds_file) {
        pstLogCtrl->uds_file = NULL;
        if (pstLogCtrl->uds_fd > 0) {
            close(pstLogCtrl->uds_fd);
            pstLogCtrl->uds_fd = -1;
        }
    }

    if (pstLogCtrl->syslog_msg_tag[0]) {
        closelog();
    }

    memset(pstLogCtrl, 0, sizeof(LOG_UTL_S));
}

void LOG_SetOutputFunc(LOG_UTL_S *pstLogCtrl, PF_LOG_OUT_FUNC output_func)
{
    pstLogCtrl->output_func = output_func;
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

void LOG_SetFileSize(LOG_UTL_S *pstLogCtrl, UINT64 filesize)
{
    pstLogCtrl->log_file_size = filesize;
}

void LOG_SetSendFailMaxCount(LOG_UTL_S *pstLogCtrl, UINT sendfailmaxcount)
{
    pstLogCtrl->send_fail_max_count = sendfailmaxcount;
}

void LOG_SetCountFile(IN LOG_UTL_S *pstLogCtrl, IN char *pcFileName)
{
    pstLogCtrl->log_count_file =
        FILE_Dup2AbsPath(pstLogCtrl->base_dir, pcFileName);
}

void LOG_SetUDSFile(IN LOG_UTL_S *pstLogCtrl, IN char *udsFileName)
{
    pstLogCtrl->uds_file = udsFileName;
}

void LOG_SetPrint(IN LOG_UTL_S *pstLogCtrl, IN BOOL_T bPrint)
{
    pstLogCtrl->print = bPrint;
}

void LOG_SetSyslog(IN LOG_UTL_S *pstLogCtrl, IN BOOL_T syslog)
{
    pstLogCtrl->syslog = syslog;
}

void LOG_SetSyslogMsgTag(IN LOG_UTL_S *pstLogCtrl, IN char *tag)
{
    if (tag == NULL || tag[0] == 0) {
        pstLogCtrl->syslog_msg_tag[0] = 0;
    } else {
        if (pstLogCtrl->syslog_opened) {
            closelog();
            pstLogCtrl->syslog_opened = 0;
        }
        strlcpy(pstLogCtrl->syslog_msg_tag, tag, sizeof(pstLogCtrl->syslog_msg_tag));
    }
}

void LOG_SetUnixSocket(IN LOG_UTL_S *pstLogCtrl, IN BOOL_T uds)
{
    pstLogCtrl->uds = uds;
}

void LOG_SetFileEnable(IN LOG_UTL_S *pstLogCtrl, IN BOOL_T enable)
{
    pstLogCtrl->file = enable;
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
    _log_Output(pstLogCtrl, str, len);
}

int Log_FillHead(char *log_type, int ver, char *buf, int buflen)
{
    time_t seconds;
    seconds = time(NULL);

    return snprintf(buf, buflen,
            "{\"type\":\"%s\",\"log_ver\":%d,\"time\":\"%s\"",
            log_type, ver, LOG_GetTimeStringBySecond(seconds));
}

int Log_FillTail(char *buf, int buflen)
{
    int i = 0;

    buf[i++] = '}';
    buf[i++] = '\n';

    return i;
}
