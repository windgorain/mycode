/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/log_utl.h"
#include "utl/txt_utl.h"
#include "utl/exec_utl.h"
#include "../h/logcenter_def.h"

static LOG_CENTER_NODE_S * logcenter_GetService(void *env)
{
    UINT index;
    CHAR *pcModeValue = CMD_EXP_GetCurrentModeValue(env);
    TXT_Atoui(pcModeValue, &index);

    return LogCenter_GetByIndex(index);
}


PLUG_API BS_STATUS LogCenter_CmdService(int argc, char **argv, void *pEnv)
{
    UINT index;
    TXT_Atoui(argv[1], &index);

    LOG_CENTER_NODE_S *svr = LogCenter_GetByIndex(index);
    
    if (! svr->used) {
        memset(svr, 0, sizeof(LOG_CENTER_NODE_S));
        svr->used = 1;
    }

    return 0;
}


PLUG_API BS_STATUS LogCenter_CmdEnable(int argc, char **argv, void *env)
{
    LOG_CENTER_NODE_S *svr = logcenter_GetService(env);

    if (argv[0][0] == 'n') {
        LogCenter_Disable(svr);
        svr->enable = 0;
        return 0;
    }

    svr->enable = 1;

    return LogCenter_Enable(svr);
}


PLUG_API BS_STATUS LogCenter_CmdSetDescription(int argc, char **argv, void *env)
{
    LOG_CENTER_NODE_S *svr = logcenter_GetService(env);
    strlcpy(svr->description, argv[1], sizeof(svr->description));

    return 0;
}


PLUG_API BS_STATUS LogCenter_CmdSetFile(int argc, char **argv, void *env)
{
    LOG_CENTER_NODE_S *svr = logcenter_GetService(env);
    strlcpy(svr->filename, argv[2], sizeof(svr->filename));

    return LogCenter_Refresh(svr);
}


PLUG_API BS_STATUS LogCenter_CmdSetFileSize(int argc, char **argv, void *env)
{
    LOG_CENTER_NODE_S *svr = logcenter_GetService(env);
    svr->log_file_size = TXT_Str2Ui(argv[2]);
    svr->log_file_size *= (1024*1024);

    return LogCenter_Refresh(svr);
}


PLUG_API BS_STATUS LogCenter_CmdSetSendFailMaxCount(int argc, char **argv, void *env)
{
    LOG_CENTER_NODE_S *svr = logcenter_GetService(env);
    svr->send_fail_max_count = TXT_Str2Ui(argv[1]);

    return LogCenter_Refresh(svr);
}


PLUG_API BS_STATUS LogCenter_CmdFileEnable(int argc, char **argv, void *env)
{
    BOOL_T enable = FALSE;
    LOG_CENTER_NODE_S *svr = logcenter_GetService(env);
    
    if (argv[0][0] != 'n') {
        enable = TRUE;
    }

    LogCenter_SetFileEnable(svr, enable);

    return BS_OK;
}


PLUG_API BS_STATUS LogCenter_CmdSetUnixSocketPath(int argc, char **argv, void *env)
{
    LOG_CENTER_NODE_S *svr = logcenter_GetService(env);
    strlcpy(svr->unixpath, argv[1], sizeof(svr->unixpath));

    return LogCenter_Refresh(svr);
}


PLUG_API BS_STATUS LogCenter_CmdSyslogEnable(int argc, char **argv, void *env)
{
    BOOL_T enable = FALSE;
    LOG_CENTER_NODE_S *svr = logcenter_GetService(env);
    
    if (argv[0][0] != 'n') {
        enable = TRUE;
    }

    LogCenter_SetSyslogEnable(svr, enable);

    return BS_OK;
}

PLUG_API BS_STATUS LogCenter_CmdSyslogTag(int argc, char **argv, void *env)
{
    LOG_CENTER_NODE_S *svr = logcenter_GetService(env);
    
    if (argv[0][0] != 'n') {
        LogCenter_SetSyslogMsgTag(svr, argv[2]);
    } else {
        LogCenter_SetSyslogMsgTag(svr, NULL);
    }

    return BS_OK;
}


PLUG_API BS_STATUS LogCenter_CmdPrintEnable(int argc, char **argv, void *env)
{
    BOOL_T enable = FALSE;
    LOG_CENTER_NODE_S *svr = logcenter_GetService(env);
    
    if (argv[0][0] != 'n') {
        enable = TRUE;
    }

    LogCenter_SetPrintEnable(svr, enable);

    return BS_OK;
}


PLUG_API BS_STATUS LogCenter_CmdUnixSocketEnable(int argc, char **argv, void *env)
{
    BOOL_T enable = FALSE;
    LOG_CENTER_NODE_S *svr = logcenter_GetService(env);

    if (argv[0][0] != 'n') {
        enable = TRUE;
    }

    LogCenter_SetUdsEnable(svr, enable);

    return 0;
}

PLUG_API BS_STATUS LogCenter_CmdShowUnixSocket(int argc, char **argv, void *env)
{
    LOG_CENTER_NODE_S *svr = logcenter_GetService(env);

    if (!svr) return -1;

    LOG_UTL_S *utl = svr->logutl;
    if (!utl) return -1;

    EXEC_OutString("Unix socket status: \r\n");
    if (utl->uds) {
        EXEC_OutInfo("Switch: %s\r\n", "Enable");
        EXEC_OutInfo("Path: %s\r\n", utl->uds_file);
    } else {
        EXEC_OutInfo("Switch: %s\r\n", "Disable");
    }
    
    EXEC_OutInfo("Num Logs Sent Total: %d\r\n", utl->log_count);
    EXEC_OutInfo("Num Logs Sent Fail: %d\r\n", utl->uds_log_fail);
    EXEC_OutInfo("Last Log Sent Reason: %s\r\n", utl->uds_last_err);

    return 0;
}

static void logcenter_SaveSvr(HANDLE file, LOG_CENTER_NODE_S *svr)
{
    if (svr->description[0]) {
        CMD_EXP_OutputCmd(file, "description %s", svr->description);
    }

    if (svr->filename[0]) {
        CMD_EXP_OutputCmd(file, "file name %s", svr->filename);
    }

    if (svr->log_file_size) {
        CMD_EXP_OutputCmd(file, "file size %llu", svr->log_file_size/(1024*1024));
    }

    if (svr->send_fail_max_count) {
        CMD_EXP_OutputCmd(file, "send_fail_max_count %u", svr->send_fail_max_count);
    }

    if (svr->file_enable) {
        CMD_EXP_OutputCmd(file, "file log enable");
    }

    if (svr->unixpath[0]) {
        CMD_EXP_OutputCmd(file, "uds-file %s", svr->unixpath);
    }

    if (svr->uds_enable) {
        CMD_EXP_OutputCmd(file, "uds enable");
    }

    if (svr->print_enable) {
        CMD_EXP_OutputCmd(file, "print enable");
    }

    if (svr->syslog_enable) {
        CMD_EXP_OutputCmd(file, "syslog enable");
    }

    if (svr->syslog_msg_tag[0]) {
        CMD_EXP_OutputCmd(file, "syslog msg_tag \'%s\'", svr->syslog_msg_tag);
    }

    if (svr->enable) {
        CMD_EXP_OutputCmd(file, "enable");
    }
}

PLUG_API BS_STATUS LogCenter_SaveCmd(HANDLE hFile)
{
    int i;

    for (i=0; i<LOG_CENTER_NODES_MAX; i++) {
        LOG_CENTER_NODE_S *svr = LogCenter_GetByIndex(i);
        if (! svr->used) {
            continue;
        }
        if (0 == CMD_EXP_OutputMode(hFile, "service %d", i)) {
            logcenter_SaveSvr(hFile, svr);
            CMD_EXP_OutputModeQuit(hFile);
        }
    }

    return BS_OK;
}

