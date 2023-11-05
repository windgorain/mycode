/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _LOGCENTER_DEF_H
#define _LOGCENTER_DEF_H
#include "utl/mysys_conf.h"
#include "utl/log_utl.h"
#ifdef __cplusplus
extern "C"
{
#endif

#define LOG_CENTER_NODES_MAX 64

typedef struct {
    char description[32];
    UINT svr_epoch: 8;
    UINT logutl_epoch: 8;
    UINT svr_id: 8;
    UINT used: 1;
    UINT enable: 1;
    UINT print_enable: 1;      
    UINT syslog_enable: 1;     
    UINT file_enable: 1;       
    UINT uds_enable: 1;        
    UINT reserved: 2;
    char filename[128];
    char unixpath[128];
    char syslog_msg_tag[128];
    UINT64 log_file_size;
    LOG_UTL_S *logutl;
    UINT send_fail_max_count;
}LOG_CENTER_NODE_S;

void LogCenter_Init();
LOG_CENTER_NODE_S * LogCenter_GetByIndex(int index);

void LogCenter_Disable(LOG_CENTER_NODE_S *svr);
int LogCenter_Enable(LOG_CENTER_NODE_S *svr);
int LogCenter_Refresh(LOG_CENTER_NODE_S *node);
void LogCenter_SetPrintEnable(LOG_CENTER_NODE_S *node, BOOL_T enable);
void LogCenter_SetSyslogEnable(LOG_CENTER_NODE_S *node, BOOL_T enable);
void LogCenter_SetSyslogMsgTag(LOG_CENTER_NODE_S *node, char *msg);
void LogCenter_SetFileEnable(LOG_CENTER_NODE_S *node, BOOL_T enable);
void LogCenter_SetUdsEnable(LOG_CENTER_NODE_S *node, BOOL_T enable);

#ifdef __cplusplus
}
#endif
#endif 
