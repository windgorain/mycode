/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _AIF_FUNC_H
#define _AIF_FUNC_H
#ifdef __cplusplus
extern "C"
{
#endif

extern BS_STATUS AIF_PCAP_Load();
int AIF_PCAP_AfterRegCmd0();
int TUN_Init();
int TAP_Init();
extern BS_STATUS AIF_VNIC_Load();
extern BS_STATUS AIF_PCAP_AutoCreateInterface();

#ifdef __cplusplus
}
#endif
#endif 
