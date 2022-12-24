#ifndef _LOG_UTL_H
#define _LOG_UTL_H
#include "utl/buffer_utl.h"
#include "utl/lpm_utl.h"
#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_LOG_BUF 2047

typedef VOID (*PF_LOG_OUT_FUNC)(void *pData, UINT uiLen);

typedef struct {
    char *base_dir;
    char *log_file_name;
    FILE *log_file_fp;
    char *log_count_file; /* 记录日志count的文件名 */
    char *uds_file;
    PF_LOG_OUT_FUNC output_func;
    UINT64 log_file_size;
    UINT64 log_count;
	UINT64 uds_log_fail;
	char  uds_last_err[128];
	char  syslog_msg_tag[256];
    int   uds_fd;
    UINT  print:1;
    UINT  syslog:1;
    UINT  file: 1;
    UINT  uds: 1;
	UINT  syslog_opened: 1;
    UINT send_fail_max_count;
}LOG_UTL_S;

typedef struct {
    UINT    modify_ts;
    char    filename[128];
    LPM_S   *lpm_handle[2];
    int     need_reload;
} SIP_MONITOR_S;

int LOG_Init(INOUT LOG_UTL_S *pstLogCtrl);
void LOG_Fini(INOUT LOG_UTL_S *pstLogCtrl);
void LOG_SetOutputFunc(LOG_UTL_S *pstLogCtrl, PF_LOG_OUT_FUNC output_func);
int LOG_ParseConfig(IN LOG_UTL_S *pstLogCtrl, IN char *pcConfigString, IN UINT uiConfigStringLen);
void LOG_SetBaseDir(IN LOG_UTL_S *pstLogCtrl, IN char *pcBaseDir);
void LOG_SetFileName(IN LOG_UTL_S *pstLogCtrl, IN char *pcFileName);
void LOG_SetFileSize(LOG_UTL_S *pstLogCtrl, UINT64 filesize);
void LOG_SetSendFailMaxCount(LOG_UTL_S *pstLogCtrl, UINT sendfailmaxcount);
void LOG_SetCountFile(IN LOG_UTL_S *pstLogCtrl, IN char *pcFileName);
void LOG_SetUDSFile(IN LOG_UTL_S *pstLogCtrl, IN char *pcUdsFileName);
void LOG_SetPrint(IN LOG_UTL_S *pstLogCtrl, IN BOOL_T bPrint);
void LOG_SetSyslogMsgTag(IN LOG_UTL_S *pstLogCtrl, IN char *pcTagMsg);
void LOG_SetUnixSocket(IN LOG_UTL_S *pstLogCtrl, IN BOOL_T uds);
void LOG_SetFileEnable(IN LOG_UTL_S *pstLogCtrl, IN BOOL_T enable);
void LOG_SetSyslog(IN LOG_UTL_S *pstLogCtrl, IN BOOL_T syslog);
char * LOG_GetTimeStringBySecond(time_t seconds);
char * LOG_GetNowTimeString();
void LOG_Output(IN LOG_UTL_S *pstLogCtrl, char *str, int len);

int Log_FillHead(char *log_type, int ver, char *buf, int buflen);
int Log_FillTail(char *buf, int buflen);

#ifdef __cplusplus
}
#endif
#endif //LOG_UTL_H_
