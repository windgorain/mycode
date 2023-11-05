/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-12-16
* Description: 
* History:     
******************************************************************************/

#ifndef __UDP_VAR_H_
#define __UDP_VAR_H_

#ifdef __cplusplus
    extern "C" {
#endif 

struct udpiphdr {
    struct ipovly    ui_i;        
    UDP_HEAD_S    ui_u;        
};
#define    ui_x1        ui_i.ih_x1
#define    ui_pr        ui_i.ih_pr
#define    ui_len       ui_i.ih_len
#define    ui_src       ui_i.ih_src
#define    ui_dst       ui_i.ih_dst
#define    ui_sport     ui_u.usSrcPort
#define    ui_dport     ui_u.usDstPort
#define    ui_ulen      ui_u.usDataLength
#define    ui_sum       ui_u.usCrc


extern PROTOSW_USER_REQUEST_S udp_usrreqs;

#ifdef __cplusplus
    }
#endif 

#endif 


