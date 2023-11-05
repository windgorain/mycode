/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2013-4-8
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/socket_utl.h"
#include "utl/eth_utl.h"
#include "utl/netinfo_utl.h"

#ifdef IN_WINDOWS

static VOID netinfo_win_FillAdapter(IN IP_ADAPTER_INFO *pstAdapter, OUT NETINFO_ADAPTER_S *pstAdapterInfo)
{
    UINT i;
    IP_ADDR_STRING *pstIpAddrString;
    
    TXT_Strlcpy(pstAdapterInfo->szAdapterName, pstAdapter->AdapterName, sizeof(pstAdapterInfo->szAdapterName));
    TXT_Strlcpy(pstAdapterInfo->szDescription, pstAdapter->Description, sizeof(pstAdapterInfo->szDescription));
    MAC_ADDR_COPY(pstAdapterInfo->stMacAddr.aucMac, pstAdapter->Address);

    pstIpAddrString = &pstAdapter->IpAddressList;
    for (i=0; i<NETINFO_ADPTER_MAX_IP_NUM; i++)
    {
        pstAdapterInfo->auiIpAddr[i] = Socket_NameToIpNet(pstIpAddrString->IpAddress.String);
        pstAdapterInfo->auiIpMask[i] = Socket_NameToIpNet(pstIpAddrString->IpMask.String);
        pstIpAddrString = pstIpAddrString->Next;
        if (pstIpAddrString == NULL)
        {
            break;
        }
    }

    pstAdapterInfo->uiGateWay = Socket_NameToIpNet(pstAdapter->GatewayList.IpAddress.String);
    pstAdapterInfo->uiDhcpServer = Socket_NameToIpNet(pstAdapter->DhcpServer.IpAddress.String);
    if (pstAdapterInfo->uiDhcpServer == 0xffffffff)
    {
        pstAdapterInfo->uiDhcpServer = 0;
    }
    
    pstAdapterInfo->bDhcpEnabled = FALSE;
    if (pstAdapter->DhcpEnabled)
    {
        pstAdapterInfo->bDhcpEnabled = TRUE;
    }
    
}

NETINFO_S * _NETINFO_GetNetInfo()
{
    IP_ADAPTER_INFO *pstAdapterInfo;
    IP_ADAPTER_INFO *pstAdapter = NULL;
    DWORD dwRetVal = 0; 
    ULONG ulOutBufLen;
    UINT uiAdapterCount = 0;
    NETINFO_S *pstNetInfo;
    UINT i = 0;

    pstAdapterInfo=(IP_ADAPTER_INFO *)MEM_ZMalloc(sizeof(IP_ADAPTER_INFO));
    if (NULL == pstAdapterInfo)
    {
        return NULL;
    }
    
    ulOutBufLen = sizeof(IP_ADAPTER_INFO);

    
    if (GetAdaptersInfo( pstAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
    {
        MEM_Free(pstAdapterInfo);
        pstAdapterInfo = (IP_ADAPTER_INFO *) MEM_ZMalloc (ulOutBufLen);
        if (NULL == pstAdapterInfo)
        {
            return NULL;
        }
    } 

    if ((dwRetVal = GetAdaptersInfo( pstAdapterInfo, &ulOutBufLen)) != NO_ERROR)
    {
        MEM_Free(pstAdapterInfo);
        return NULL;
    }

    pstAdapter = pstAdapterInfo; 
    
    while (pstAdapter)
    { 
        if ((pstAdapter->Type == MIB_IF_TYPE_ETHERNET) || (pstAdapter->Type == IF_TYPE_IEEE80211))
        {
            uiAdapterCount ++;
        }
        pstAdapter = pstAdapter->Next;
    }

    pstNetInfo = MEM_ZMalloc(sizeof(NETINFO_S) + sizeof(NETINFO_ADAPTER_S) * uiAdapterCount);
    if (NULL == pstNetInfo)
    {
        MEM_Free(pstAdapterInfo);
        return NULL;
    }

    pstNetInfo->uiAdapterNum = uiAdapterCount;

    pstAdapter = pstAdapterInfo; 
    while (pstAdapter)
    { 
        if ((pstAdapter->Type == MIB_IF_TYPE_ETHERNET) || (pstAdapter->Type == IF_TYPE_IEEE80211))
        {
            netinfo_win_FillAdapter(pstAdapter, &pstNetInfo->astAdapter[i]);
            i++;
        }
        pstAdapter = pstAdapter->Next;
    }

    MEM_Free(pstAdapterInfo);

    return pstNetInfo;
}

#endif

#ifdef IN_UNIXLIKE

#ifdef IN_LINUX
#include <linux/if.h>
#endif

#ifdef IN_MAC
#include <net/if.h>
#endif

UINT get_if_ip(IN char *ifname)
{  
    int sock_get_ip;  
    struct   sockaddr_in *sin;  
    struct   ifreq ifr_ip;     
  
    if ((sock_get_ip = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
         return 0;
    }  
     
    memset(&ifr_ip, 0, sizeof(ifr_ip));     
    strncpy(ifr_ip.ifr_name, "eth0", sizeof(ifr_ip.ifr_name) - 1);     
    if( ioctl( sock_get_ip, SIOCGIFADDR, &ifr_ip) < 0 ) {     
        close(sock_get_ip);
         return 0;
    }
    close( sock_get_ip );  

    sin = (struct sockaddr_in *)&ifr_ip.ifr_addr;     

    return sin->sin_addr.s_addr;
}

#ifndef IN_MAC

MAC_ADDR_S * get_if_mac(IN char *ifname, OUT MAC_ADDR_S *mac)  
{
    int sock_mac;
    struct ifreq ifr_mac;  
 
    sock_mac = socket( AF_INET, SOCK_STREAM, 0 );  
    if( sock_mac == -1)  {
        return NULL;
    }  
      
    memset(&ifr_mac,0,sizeof(ifr_mac));     
    strncpy(ifr_mac.ifr_name, ifname, sizeof(ifr_mac.ifr_name)-1);     
  
    if( (ioctl( sock_mac, SIOCGIFHWADDR, &ifr_mac)) < 0)  {
        close( sock_mac );  
        return NULL;
    }  

    mac->aucMac[0] = ifr_mac.ifr_hwaddr.sa_data[0];
    mac->aucMac[1] = ifr_mac.ifr_hwaddr.sa_data[1];
    mac->aucMac[2] = ifr_mac.ifr_hwaddr.sa_data[2];
    mac->aucMac[3] = ifr_mac.ifr_hwaddr.sa_data[3];
    mac->aucMac[4] = ifr_mac.ifr_hwaddr.sa_data[4];
    mac->aucMac[5] = ifr_mac.ifr_hwaddr.sa_data[5];

    close( sock_mac );  

    return mac;
}

UINT get_if_mask(IN char *ifname)
{  
    int sock_netmask;  
    struct ifreq ifr_mask;  
    struct sockaddr_in *net_mask;  
          
    sock_netmask = socket( AF_INET, SOCK_STREAM, 0 );  
    if( sock_netmask == -1) { 
        return 0;
    }  
      
    memset(&ifr_mask, 0, sizeof(ifr_mask));     
    strncpy(ifr_mask.ifr_name, ifname, sizeof(ifr_mask.ifr_name )-1);     
  
    if( (ioctl( sock_netmask, SIOCGIFNETMASK, &ifr_mask ) ) < 0 )   { 
        close(sock_netmask);
        return 0;
    }
    close( sock_netmask );  

    net_mask = ( struct sockaddr_in * )&( ifr_mask.ifr_netmask);  

    return net_mask->sin_addr.s_addr;
}
#endif

static VOID _netinfo_FillInfo(OUT NETINFO_ADAPTER_S *pstNetInfo, IN CHAR *pcName)
{

    TXT_Strlcpy(pstNetInfo->szAdapterName, pcName, NETINFO_ADATER_NAME_MAX_LEN + 1);

    pstNetInfo->auiIpAddr[0] = get_if_ip(pcName);

#ifndef IN_MAC
    {
        MAC_ADDR_S mac;

        pstNetInfo->auiIpMask[0] = get_if_mask(pcName);
        if (NULL != get_if_mac(pcName, &mac)) {
            pstNetInfo->stMacAddr = mac;
        }
    }
#endif

    return;    
}

NETINFO_S * _NETINFO_GetNetInfo()
{
    INT iSocketID;
    struct ifconf stIfconf;
    struct ifreq astIfReqs[128];
    INT i, iReqCount;
    NETINFO_S *pstNetInfos;
    CHAR *pcName;
    CHAR *pcPreName = "";
    int ret;

    iSocketID = Socket_Create(AF_INET, SOCK_DGRAM);
    if (iSocketID < 0) {
        return NULL;
    }

    stIfconf.ifc_len = sizeof(astIfReqs);
    stIfconf.ifc_buf = (VOID*)astIfReqs;

    ret = ioctl(iSocketID, SIOCGIFCONF, &stIfconf);
    Socket_Close(iSocketID);

    if (ret < 0) {
        return NULL;
    }

    iReqCount = stIfconf.ifc_len/sizeof(struct ifreq);

    pstNetInfos = MEM_ZMalloc(sizeof(NETINFO_S) + sizeof(NETINFO_ADAPTER_S) * iReqCount);
    if (NULL == pstNetInfos) {
        return NULL;
    }

    for (i=0; i<iReqCount; i++) {
        pcName = astIfReqs[i].ifr_name;

        if (strcmp(pcName, pcPreName) == 0) {
            continue;
        }

        _netinfo_FillInfo(&pstNetInfos->astAdapter[pstNetInfos->uiAdapterNum], pcName);

        pcPreName = pcName;
        pstNetInfos->uiAdapterNum ++;
    }

    return pstNetInfos;
}

#endif

