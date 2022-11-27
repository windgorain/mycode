/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/atomic_utl.h"
#include "utl/spinlock_utl.h"
#include "utl/log_utl.h"
#include "utl/file_utl.h"
#include "utl/atomic_utl.h"
#include "utl/file_utl.h"
#include "../h/logcenter_def.h"

static LOG_CENTER_NODE_S g_log_center_nodes[LOG_CENTER_NODES_MAX];

void LogCenter_Init()
{
    UINT i;

    for (i=0; i<LOG_CENTER_NODES_MAX; i++) {
        g_log_center_nodes[i].svr_id = i;
    }
}

LOG_CENTER_NODE_S * LogCenter_GetByIndex(int index)
{
    if (index >= LOG_CENTER_NODES_MAX) {
        return NULL;
    }

    return &g_log_center_nodes[index];
}

void LogCenter_SetPrintEnable(LOG_CENTER_NODE_S *node, BOOL_T enable)
{
    node->print_enable = enable;
    if (node->logutl) {
        LOG_SetPrint(node->logutl, enable);
    }
}

void LogCenter_SetSyslogEnable(LOG_CENTER_NODE_S *node, BOOL_T enable)
{
    node->syslog_enable = enable;
    if (node->logutl) {
        LOG_SetSyslog(node->logutl, enable);
    }
}

void LogCenter_SetSyslogMsgTag(LOG_CENTER_NODE_S *node, char *tag) 
{
    if (tag && tag[0]) {
        strlcpy(node->syslog_msg_tag, tag, sizeof(node->syslog_msg_tag));
    }

    if (node->logutl) {
        LOG_SetSyslogMsgTag(node->logutl, tag);
    }
}

void LogCenter_SetFileEnable(LOG_CENTER_NODE_S *node, BOOL_T enable)
{
    node->file_enable = enable;
    if (node->logutl) {
        LOG_SetFileEnable(node->logutl, enable);
    }
}

void LogCenter_SetUdsEnable(LOG_CENTER_NODE_S *node, BOOL_T enable)
{
    node->uds_enable = enable;
    if (node->logutl) {
        LOG_SetUnixSocket(node->logutl, enable);
    }
}

void LogCenter_Disable(LOG_CENTER_NODE_S *svr)
{
    LOG_UTL_S *logutl = svr->logutl;

    if (logutl == NULL) {
        return;
    }

    svr->logutl = NULL;

    Sleep(1000);

    LOG_Fini(logutl);
    MEM_Free(logutl);
}

int LogCenter_Enable(LOG_CENTER_NODE_S *svr)
{
    LOG_UTL_S *logutl = NULL;
    if (svr->logutl) {
        return 0;
    }

    logutl = MEM_ZMalloc(sizeof(LOG_UTL_S));
    if (logutl) {
        LOG_Init(logutl);

        LOG_SetBaseDir(logutl, "log");
        LOG_SetPrint(logutl, svr->print_enable);
        LOG_SetSyslog(logutl, svr->syslog_enable);
        LOG_SetSyslogMsgTag(logutl, svr->syslog_msg_tag);
        LOG_SetFileEnable(logutl, svr->file_enable);
        LOG_SetUnixSocket(logutl, svr->uds_enable);

        char buf[FILE_MAX_PATH_LEN + 1];
        if (FILE_IsAbsolutePath(svr->filename)) {
            scnprintf(buf, sizeof(buf), "%s", svr->filename);
        } else {
            scnprintf(buf, sizeof(buf), "%d/%s", svr->svr_id, svr->filename);
        }
        LOG_SetFileName(logutl, buf);
        LOG_SetFileSize(logutl, svr->log_file_size);
        if(svr->send_fail_max_count){
            LOG_SetSendFailMaxCount(logutl,svr->send_fail_max_count);
        }

        if (svr->unixpath[0]) {
            LOG_SetUDSFile(logutl, svr->unixpath);
        }
    }

    svr->logutl = logutl;

    if (! logutl) {
        RETURN(BS_ERR);
    }

    return 0;
}

int LogCenter_Refresh(LOG_CENTER_NODE_S *svr)
{
    int ret = 0;

    LogCenter_Disable(svr);

    if (svr->enable) {
        ret = LogCenter_Enable(svr);
    }

    return ret;
}

PLUG_API void LogCenter_OutputString(UINT service, char *string)
{
    LOG_CENTER_NODE_S *svr;
    int len;

    if (service >= LOG_CENTER_NODES_MAX) {
        return;
    }

    len = strlen(string);
    if (len == 0) {
        return;
    }

    svr = &g_log_center_nodes[service];

    LOG_UTL_S *logutl = svr->logutl;
    if (logutl) {
        LOG_Output(logutl, string, len);
    }
}

PLUG_API void LogCenter_OutputStringExt(UINT service, char *string, int length)
{
    LOG_CENTER_NODE_S *svr;

    if (service >= LOG_CENTER_NODES_MAX) {
        return;
    }

    if (length == 0) {
        return;
    }

    svr = &g_log_center_nodes[service];

    LOG_UTL_S *logutl = svr->logutl;
    if (logutl) {
        LOG_Output(logutl, string, length);
    }
}

PLUG_API void LogCenter_OutputInfo(UINT service, char *fmt, ...)
{
    char buf[2048];
    TXT_ARGS_BUILD(buf, sizeof(buf));
    LogCenter_OutputString(service, buf);
}

PLUG_API void LogCenter_OutputArgs(UINT service, char *fmt, va_list args)
{
    char buf[2048];
    BS_Vsnprintf(buf, sizeof(buf), fmt, args);
    LogCenter_OutputString(service, buf);
}

