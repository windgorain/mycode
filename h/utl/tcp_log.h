/*================================================================
*   Description:
*
================================================================*/
#ifndef _TCP_LOG_H
#define _TCP_LOG_H

#include "utl/log_utl.h"
#include "utl/tcp_finger.h"
#include "utl/bloomfilter_utl.h"
#include "utl/box_utl.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define TCP_LOG_ENABLE(log) (log.log_enable)

typedef struct {
    UINT log_enable: 1;
    UINT log_all_pkt: 1;
    UINT ip_header_hex: 1;
    UINT tcp_header_hex: 1;
    UINT os_finger: 1;
    UINT only_sip_enable: 1;
	UINT tcp_log_enable: 1;
    UINT white_ip_enable: 1;
    USHORT tcp_payload_hex;
    char *config_base_dir;
    LOG_UTL_S log_utl;
    TCP_FINGER_S tcp_finger;
    BLOOM_FILTER_S bloomfilter;
    SIP_MONITOR_S sip_monitor;
    BOX_S white_ip_list;
    UINT bloomfilter_size; 
}TCP_LOG_S;

void TCPLOG_Init(TCP_LOG_S *config);
void TCPLOG_SetDir(TCP_LOG_S *config, char *conf_base_dir, char *log_base_dir);
void TCPLOG_Final(TCP_LOG_S *config);
void TCPLOG_SetOutputFunc(TCP_LOG_S *config, PF_LOG_OUT_FUNC output_func);
BS_STATUS TCPLOG_ParseConfig(TCP_LOG_S *config, char *conf_string);
int TCPLOG_Input(TCP_LOG_S *config, VOID *ippkt, UINT pktlen, NET_PKT_TYPE_E pkt_type, int dir);

#ifdef __cplusplus
}
#endif
#endif 
