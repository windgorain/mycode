#define UNICODE 
#define _UNICODE 

#include <Winsock2.h> 
#include <Ws2spi.h> 
#include <Windows.h> 
#include <tchar.h> 
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#pragma comment(lib, "Ws2_32.lib") 
#pragma comment(lib, "winmm.lib") 

typedef enum
{
	SSLVPNLSP_FLAG_RELAY,			/* ��Ҫ���� */
	SSLVPNLSP_FLAG_NO_RELAY			/* ����Ҫ���� */
};

typedef struct
{
	UINT uiStartIp;
	UINT uiStopIp;
	USHORT usStartPort;
	USHORT usStopPort;
}SSLVPNLSP_FILE_NODE_S;

static WSPUPCALLTABLE g_pUpCallTable;      // �ϲ㺯���б����LSP�������Լ���α�������ʹ����������б� 
static WSPPROC_TABLE g_NextProcTable;      // �²㺯���б� 
static TCHAR   g_szCurrentApp[MAX_PATH];   // ��ǰ���ñ�DLL�ĳ�������� 
static CRITICAL_SECTION g_stSslvpnlspMutex;
static SSLVPNLSP_FILE_NODE_S g_astSslvpnlspFileMap[512];
static UINT g_uiSslvpnlspNodeCount = 0;

BOOL APIENTRY DllMain( HANDLE hModule,  
                       DWORD  ul_reason_for_call,  
                       LPVOID lpReserved 
                     ) 
{ 
    switch (ul_reason_for_call) 
    { 
    case DLL_PROCESS_ATTACH: 
        { 
            // ȡ����ģ������� 
            ::GetModuleFileName(NULL, g_szCurrentApp, MAX_PATH); 
        } 
        break; 
    } 

	InitializeCriticalSection(&g_stSslvpnlspMutex);

    return TRUE; 
} 
 
 
LPWSAPROTOCOL_INFOW GetProvider(LPINT lpnTotalProtocols) 
{ 
    DWORD dwSize = 0; 
    int nError; 
    LPWSAPROTOCOL_INFOW pProtoInfo = NULL; 
     
    // ȡ����Ҫ�ĳ��� 
    if(::WSCEnumProtocols(NULL, pProtoInfo, &dwSize, &nError) == SOCKET_ERROR) 
    { 
        if(nError != WSAENOBUFS) 
            return NULL; 
    } 
     
    pProtoInfo = (LPWSAPROTOCOL_INFOW)::GlobalAlloc(GPTR, dwSize); 
    *lpnTotalProtocols = ::WSCEnumProtocols(NULL, pProtoInfo, &dwSize, &nError); 
    return pProtoInfo; 
} 
 
void FreeProvider(LPWSAPROTOCOL_INFOW pProtoInfo) 
{ 
    ::GlobalFree(pProtoInfo); 
}

#define _SSLVPNLSP_TCP_NODE_FILE_NAME "c:\\sslvpnlsp.data"
#define MIN(a,b)  ((a)<(b) ? (a) : (b))

/* ���ļ����ݿ�����ָ�������� */
static void _sslvpnlsp_MemMap(IN UINT uiFileSize)
{
    FILE *fp;
    UINT uiReadLen;

	g_uiSslvpnlspNodeCount = 0;

    uiReadLen = MIN(sizeof(g_astSslvpnlspFileMap), uiFileSize);

    fp = fopen(_SSLVPNLSP_TCP_NODE_FILE_NAME, "rb");
    if (NULL == fp)
    {
        return;
    }

    fread(g_astSslvpnlspFileMap, 1, uiReadLen, fp);

    fclose(fp);

	g_uiSslvpnlspNodeCount = sizeof(g_astSslvpnlspFileMap)/sizeof(SSLVPNLSP_FILE_NODE_S);

    return;
}

static VOID _sslvpnlsp_ReadTcpRelayFile()
{
	static UINT uiOldFileTime = 0;
	UINT uiFileTime;
	UINT uiFileSize;
	struct stat f_stat;

    if (stat(_SSLVPNLSP_TCP_NODE_FILE_NAME, &f_stat ) == -1)
    {
    	g_uiSslvpnlspNodeCount = 0;
        return;
    }

    uiFileTime = (UINT)f_stat.st_mtime;
	uiFileSize = (UINT)f_stat.st_size;

	if (uiFileTime == uiOldFileTime)
	{
		return;
	}

	uiOldFileTime = uiFileTime;

	_sslvpnlsp_MemMap(uiFileSize);
}

static UINT _sslvpnlsp_TestIP(IN UINT uiIP/* ������ */)
{
	UINT i;

	for (i=0; i<g_uiSslvpnlspNodeCount; i++)
	{
		if ((uiIP >= g_astSslvpnlspFileMap[i].uiStartIp)
			&& (uiIP <= g_astSslvpnlspFileMap[i].uiStopIp))
		{
			return SSLVPNLSP_FLAG_RELAY;
		}
	}

	return SSLVPNLSP_FLAG_NO_RELAY;
}

static UINT _sslvpnlsp_TestRelay(IN UINT uiIP/* ������ */)
{
	UINT uiReslut;

	EnterCriticalSection(&g_stSslvpnlspMutex);
	_sslvpnlsp_ReadTcpRelayFile();
	uiReslut = _sslvpnlsp_TestIP(uiIP);
	LeaveCriticalSection(&g_stSslvpnlspMutex);

	return uiReslut;
}

static UINT _sslvpnlsp_CheckRelayPermit(const struct sockaddr FAR * name)
{
	UINT uiDstIP;
	struct sockaddr_in *pstSockAddr;

	if (name->sa_family != AF_INET)
	{
		return SSLVPNLSP_FLAG_NO_RELAY;
	}

	pstSockAddr = (struct sockaddr_in *)(void*)name;
	uiDstIP = ntohl(pstSockAddr->sin_addr.s_addr);

	if ((uiDstIP & 0xff000000) == 0x7f000000)
    {
        return SSLVPNLSP_FLAG_NO_RELAY;
    }
	
	return _sslvpnlsp_TestRelay(uiDstIP);
}

static INT _sslvpnlsp_Start
(
    SOCKET s,
    const struct sockaddr FAR * name,
    int namelen,
    LPWSABUF lpCallerData,
    LPWSABUF lpCalleeData,
    LPQOS lpSQOS,
    LPQOS lpGQOS,
    LPINT lpErrno
)
{
	struct sockaddr_in *pstSockAddr;
	UINT ulIoMode = 0;
	INT iRet;
	UINT uiOldIP;
	USHORT usOldPort;
	CHAR szInfo[128];

	pstSockAddr = (struct sockaddr_in *)(void*)name;

	uiOldIP = pstSockAddr->sin_addr.s_addr;
	usOldPort = pstSockAddr->sin_port;

	pstSockAddr->sin_addr.s_addr = htonl(0x7f000001);
	pstSockAddr->sin_port = htons(60002);

	iRet = g_NextProcTable.lpWSPConnect(s, name, namelen, lpCallerData, lpCalleeData, lpSQOS, lpGQOS, lpErrno);

	sprintf(szInfo, "Type=HSK\r\nIP=%x\r\nPort=%d\r\n\r\n", ntohl(uiOldIP), ntohs(usOldPort));

	send(s, szInfo, strlen(szInfo), 0);

	return iRet;
}

int WSPAPI WSPConnect
(
    SOCKET s,
    const struct sockaddr FAR * name,
    int namelen,
    LPWSABUF lpCallerData,
    LPWSABUF lpCalleeData,
    LPQOS lpSQOS,
    LPQOS lpGQOS,
    LPINT lpErrno
)
{
	int iRet;

	if (SSLVPNLSP_FLAG_NO_RELAY == _sslvpnlsp_CheckRelayPermit(name))
	{
		iRet = g_NextProcTable.lpWSPConnect(s, name, namelen, lpCallerData, lpCalleeData, lpSQOS, lpGQOS, lpErrno);
	}
	else
	{
		iRet = _sslvpnlsp_Start(s, name, namelen, lpCallerData, lpCalleeData, lpSQOS, lpGQOS, lpErrno);
	}

	return iRet;
}

int WSPAPI WSPStartup( 
  WORD wVersionRequested, 
  LPWSPDATA lpWSPData, 
  LPWSAPROTOCOL_INFO lpProtocolInfo, 
  WSPUPCALLTABLE UpcallTable, 
  LPWSPPROC_TABLE lpProcTable 
) 
{ 
	int i;
     
    if(lpProtocolInfo->ProtocolChain.ChainLen <= 1) 
    {    
        return WSAEPROVIDERFAILEDINIT; 
    } 
     
    // �������ϵ��õĺ�����ָ�루�������ǲ�ʹ������ 
    g_pUpCallTable = UpcallTable; 
 
    // ö��Э�飬�ҵ��²�Э���WSAPROTOCOL_INFOW�ṹ   
    WSAPROTOCOL_INFOW   NextProtocolInfo; 
    int nTotalProtos; 
    LPWSAPROTOCOL_INFOW pProtoInfo = GetProvider(&nTotalProtos); 
    // �²����ID    
    DWORD dwBaseEntryId = lpProtocolInfo->ProtocolChain.ChainEntries[1]; 
    for(i=0; i<nTotalProtos; i++) 
    { 
        if(pProtoInfo[i].dwCatalogEntryId == dwBaseEntryId) 
        { 
            memcpy(&NextProtocolInfo, &pProtoInfo[i], sizeof(NextProtocolInfo)); 
            break; 
        } 
    } 
    if(i >= nTotalProtos) 
    { 
        printf(" WSPStartup:  Can not find underlying protocol \n"); 
        return WSAEPROVIDERFAILEDINIT; 
    } 
 
    // �����²�Э���DLL 
    int nError; 
    TCHAR szBaseProviderDll[MAX_PATH]; 
    int nLen = MAX_PATH; 
    // ȡ���²��ṩ����DLL·�� 
    if(::WSCGetProviderPath(&NextProtocolInfo.ProviderId, szBaseProviderDll, &nLen, &nError) == SOCKET_ERROR) 
    { 
        printf(" WSPStartup: WSCGetProviderPath() failed %d \n", nError); 
        return WSAEPROVIDERFAILEDINIT; 
    } 

    if(!::ExpandEnvironmentStrings(szBaseProviderDll, szBaseProviderDll, MAX_PATH)) 
    { 
        printf(" WSPStartup:  ExpandEnvironmentStrings() failed %d \n", ::GetLastError()); 
        return WSAEPROVIDERFAILEDINIT; 
    } 

    // �����²��ṩ���� 
    HMODULE hModule = ::LoadLibrary(szBaseProviderDll); 
    if(hModule == NULL) 
    { 
        printf(" WSPStartup:  LoadLibrary() failed %d \n", ::GetLastError()); 
        return WSAEPROVIDERFAILEDINIT; 
    } 

    // �����²��ṩ�����WSPStartup���� 
    LPWSPSTARTUP  pfnWSPStartup = NULL; 
    pfnWSPStartup = (LPWSPSTARTUP)::GetProcAddress(hModule, "WSPStartup"); 
    if(pfnWSPStartup == NULL) 
    { 
        printf(" WSPStartup:  GetProcAddress() failed %d \n", ::GetLastError()); 
        return WSAEPROVIDERFAILEDINIT; 
    } 
 
    // �����²��ṩ�����WSPStartup���� 
    LPWSAPROTOCOL_INFOW pInfo = lpProtocolInfo; 
    if(NextProtocolInfo.ProtocolChain.ChainLen == BASE_PROTOCOL) 
        pInfo = &NextProtocolInfo; 
 
    int nRet = pfnWSPStartup(wVersionRequested, lpWSPData, pInfo, UpcallTable, lpProcTable); 
    if(nRet != ERROR_SUCCESS) 
    { 
        printf(" WSPStartup:  underlying provider's WSPStartup() failed %d \n", nRet); 
        return nRet; 
    } 
 
    // �����²��ṩ�ߵĺ����� 
    g_NextProcTable = *lpProcTable; 
 
    // �޸Ĵ��ݸ��ϲ�ĺ�����Hook����Ȥ�ĺ�����������Ϊʾ������Hook��WSPSendTo���� 
    // ��������Hook������������WSPSocket��WSPCloseSocket��WSPConnect�� 
	lpProcTable->lpWSPConnect = WSPConnect;
 
    FreeProvider(pProtoInfo); 
    return nRet; 
} 


