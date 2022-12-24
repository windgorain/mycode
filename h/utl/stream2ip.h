/*================================================================
*   Description：
*
================================================================*/
#ifndef _STREAM2IP_H
#define _STREAM2IP_H

#include "utl/ip_utl.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define S2IP_DATA_FLAG_BIT_REPLY 0x1
#define S2IP_DATA_FLAG_BIT_FIN   0x2

typedef struct {
    MAC_ADDR_S smac;
    MAC_ADDR_S dmac;
    unsigned int sip; /*net order*/
    unsigned int dip;
    unsigned short sport;
    unsigned short dport;
    unsigned char protocol;
    unsigned int seq;
    unsigned int ack; /* 反向数据偏移 */
}S2IP_S;

typedef int (*PF_S2IP_OUTPUT)(void *pkt, int pkt_len, void *user_handle);

void S2IP_Init(OUT S2IP_S *s2ip_ctrl, UINT sip, UINT dip, USHORT sport, USHORT dport);
void S2IP_SetMac(OUT S2IP_S *s2ip_ctrl, MAC_ADDR_S *smac, MAC_ADDR_S *dmac);
void S2IP_Switch(S2IP_S *s2ip_ctrl);

/* 三次握手 */
int S2IP_Hsk(S2IP_S *s2ip_ctrl, PF_S2IP_OUTPUT output_func, void *user_handle);

/* 数据报文 */
int S2IP_Data(S2IP_S *ctrl, void *data, int data_len, BOOL_T ack, PF_S2IP_OUTPUT output_func, void *user_handle);

/* 四次挥手 */
int S2IP_Bye(S2IP_S *s2ip_ctrl, PF_S2IP_OUTPUT output_func, void *user_handle);

#ifdef __cplusplus
}
#endif
#endif //STREAM2IP_H_
