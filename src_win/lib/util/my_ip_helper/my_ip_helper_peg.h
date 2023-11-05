/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-7-6
* Description: 桩文件
* History:     
******************************************************************************/

#桩文件,禁止引用

typedef struct _IP_ADAPTER_ADDRESSES
{
    union
    {
        ULONGLONG Alignment;
        struct
        {
            ULONG Length;
            DWORD IfIndex;
        };
    };
    struct _IP_ADAPTER_ADDRESSES  *Next;
    CHAR * AdapterName;
    PIP_ADAPTER_UNICAST_ADDRESS FirstUnicastAddress;
    PIP_ADAPTER_ANYCAST_ADDRESS FirstAnycastAddress;
    PIP_ADAPTER_MULTICAST_ADDRESS FirstMulticastAddress;
    PIP_ADAPTER_DNS_SERVER_ADDRESS FirstDnsServerAddress;
    PWCHAR DnsSuffix;
    PWCHAR Description;
    PWCHAR FriendlyName;
    BYTE PhysicalAddress[MAX_ADAPTER_ADDRESS_LENGTH];
    DWORD PhysicalAddressLength;
    DWORD Flags;
    DWORD Mtu;
    DWORD IfType;
    IF_OPER_STATUS OperStatus;
    DWORD Ipv6IfIndex;
    DWORD ZoneIndices[16];
    PIP_ADAPTER_PREFIX FirstPrefix;
    ULONG64 TransmitLinkSpeed;
    ULONG64 ReceiveLinkSpeed;
    PIP_ADAPTER_WINS_SERVER_ADDRESS_LH FirstWinsServerAddress;
    PIP_ADAPTER_GATEWAY_ADDRESS_LH FirstGatewayAddress;
    ULONG Ipv4Metric;
    ULONG Ipv6Metric;
    IF_LUID Luid;
    SOCKET_ADDRESS Dhcpv4Server;
    NET_IF_COMPARTMENT_ID CompartmentId;
    NET_IF_NETWORK_GUID NetworkGuid;
    NET_IF_CONNECTION_TYPE ConnectionType;
    TUNNEL_TYPE TunnelType;
    SOCKET_ADDRESS Dhcpv6Server;
    BYTE Dhcpv6ClientDuid[MAX_DHCPV6_DUID_LENGTH];
    ULONG Dhcpv6ClientDuidLength;
    ULONG Dhcpv6Iaid;
    PIP_ADAPTER_DNS_SUFFIX FirstDnsSuffix;
} IP_ADAPTER_ADDRESSES;

typedef struct _MIB_IPFORWARDROW
{
    DWORD dwForwardDest;        
    DWORD dwForwardMask;        
    DWORD dwForwardPolicy;      
    DWORD dwForwardNextHop;     
    DWORD dwForwardIfIndex;     
    DWORD dwForwardType;        
    DWORD dwForwardProto;       
    DWORD dwForwardAge;         
    DWORD dwForwardNextHopAS;   
    DWORD dwForwardMetric1;
    DWORD dwForwardMetric2; 
    DWORD dwForwardMetric3;
    DWORD dwForwardMetric4;
    DWORD dwForwardMetric5; 
} MIB_IPFORWARDROW, *PMIB_IPFORWARDROW;

typedef struct {
    char String[4 * 4];
} IP_ADDRESS_STRING, *PIP_ADDRESS_STRING, IP_MASK_STRING, *PIP_MASK_STRING;

typedef struct _IP_ADDR_STRING
{
    struct _IP_ADDR_STRING* Next;
    IP_ADDRESS_STRING IpAddress;
    IP_MASK_STRING IpMask;
    DWORD Context;
} IP_ADDR_STRING, *PIP_ADDR_STRING;

typedef struct _IP_ADAPTER_INFO
{
    struct _IP_ADAPTER_INFO* Next;
    DWORD ComboIndex;
    Char AdapterName[MAX_ADAPTER_NAME_LENGTH + 4];
    char Description[MAX_ADAPTER_DESCRIPTION_LENGTH + 4];
    UINT AddressLength;
    BYTE Address[MAX_ADAPTER_ADDRESS_LENGTH];
    DWORD Index;
    UINT Type;
    UINT DhcpEnabled;
    PIP_ADDR_STRING CurrentIpAddress;
    IP_ADDR_STRING IpAddressList;
    IP_ADDR_STRING GatewayList;
    IP_ADDR_STRING DhcpServer;
    BOOL HaveWins;
    IP_ADDR_STRING PrimaryWinsServer;
    IP_ADDR_STRING SecondaryWinsServer;
    time_t LeaseObtained;
    time_t LeaseExpires;
} IP_ADAPTER_INFO, *PIP_ADAPTER_INFO;



typedef
BOOL
(CALLBACK FAR * LPBLOCKINGCALLBACK)(
    DWORD_PTR dwContext
    );



typedef
VOID
(CALLBACK FAR * LPWSAUSERAPC)(
    DWORD_PTR dwContext
    );



typedef
__checkReturn
SOCKET
(WSPAPI * LPWSPACCEPT)(
    __in SOCKET s,
    __out_bcount_part_opt(*addrlen, *addrlen) struct sockaddr FAR * addr,
    __inout_opt LPINT addrlen,
    __in_opt LPCONDITIONPROC lpfnCondition,
    __in_opt DWORD_PTR dwCallbackData,
    __out LPINT lpErrno
    );

typedef
INT
(WSPAPI * LPWSPADDRESSTOSTRING)(
    __in_bcount(dwAddressLength) LPSOCKADDR lpsaAddress,
    __in DWORD dwAddressLength,
    __in_opt LPWSAPROTOCOL_INFOW lpProtocolInfo,
    __out_ecount_part(*lpdwAddressStringLength, *lpdwAddressStringLength) LPWSTR lpszAddressString,
    __inout LPDWORD lpdwAddressStringLength,
    __out LPINT lpErrno
    );

typedef
int
(WSPAPI * LPWSPASYNCSELECT)(
    __in SOCKET s,
    __in HWND hWnd,
    __in unsigned int wMsg,
    __in long lEvent,
    __out LPINT lpErrno
    );

typedef
int
(WSPAPI * LPWSPBIND)(
    __in SOCKET s,
    __in_bcount(namelen) const struct sockaddr FAR * name,
    __in int namelen,
    __out LPINT lpErrno
    );

typedef
int
(WSPAPI * LPWSPCANCELBLOCKINGCALL)(
    __out LPINT lpErrno
    );

typedef
int
(WSPAPI * LPWSPCLEANUP)(
    __out LPINT lpErrno
    );

typedef
int
(WSPAPI * LPWSPCLOSESOCKET)(
    __in SOCKET s,
    __out LPINT lpErrno
    );

typedef
int
(WSPAPI * LPWSPCONNECT)(
    __in SOCKET s,
    __in_bcount(namelen) const struct sockaddr FAR * name,
    __in int namelen,
    __in_opt LPWSABUF lpCallerData,
    __out_opt LPWSABUF lpCalleeData,
    __in_opt LPQOS lpSQOS,
    __in_opt LPQOS lpGQOS,
    __out LPINT lpErrno
    );

typedef
int
(WSPAPI * LPWSPDUPLICATESOCKET)(
    __in SOCKET s,
    __in DWORD dwProcessId,
    __out LPWSAPROTOCOL_INFOW lpProtocolInfo,
    __out LPINT lpErrno
    );

typedef
int
(WSPAPI * LPWSPENUMNETWORKEVENTS)(
    __in SOCKET s,
    __in WSAEVENT hEventObject,
    __out LPWSANETWORKEVENTS lpNetworkEvents,
    __out LPINT lpErrno
    );

typedef
int
(WSPAPI * LPWSPEVENTSELECT)(
    __in SOCKET s,
    __in WSAEVENT hEventObject,
    __in long lNetworkEvents,
    __out LPINT lpErrno
    );

typedef
BOOL
(WSPAPI * LPWSPGETOVERLAPPEDRESULT)(
    __in SOCKET s,
    __in LPWSAOVERLAPPED lpOverlapped,
    __out LPDWORD lpcbTransfer,
    __in BOOL fWait,
    __out LPDWORD lpdwFlags,
    __out LPINT lpErrno
    );

typedef
int
(WSPAPI * LPWSPGETPEERNAME)(
    __in SOCKET s,
    __out_bcount_part(*namelen, *namelen) struct sockaddr FAR * name,
    __inout LPINT namelen,
    __out LPINT lpErrno
    );

typedef
int
(WSPAPI * LPWSPGETSOCKNAME)(
    __in SOCKET s,
    __out_bcount_part(*namelen, *namelen) struct sockaddr FAR * name,
    __inout LPINT namelen,
    __out LPINT lpErrno
    );

typedef
int
(WSPAPI * LPWSPGETSOCKOPT)(
    __in SOCKET s,
    __in int level,
    __in int optname,
    __out_bcount(*optlen) char FAR * optval,
    __inout LPINT optlen,
    __out LPINT lpErrno
    );

typedef
BOOL
(WSPAPI * LPWSPGETQOSBYNAME)(
    __in SOCKET s,
    __in LPWSABUF lpQOSName,
    __out LPQOS lpQOS,
    __out LPINT lpErrno
    );

typedef
int
(WSPAPI * LPWSPIOCTL)(
    __in SOCKET s,
    __in DWORD dwIoControlCode,
    __in_bcount_opt(cbInBuffer) LPVOID lpvInBuffer,
    __in DWORD cbInBuffer,
    __out_bcount_part_opt(cbOutBuffer, *lpcbBytesReturned) LPVOID lpvOutBuffer,
    __in DWORD cbOutBuffer,
    __out LPDWORD lpcbBytesReturned,
    __inout_opt LPWSAOVERLAPPED lpOverlapped,
    __in_opt LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    __in_opt LPWSATHREADID lpThreadId,
    __out LPINT lpErrno
    );

typedef
SOCKET
(WSPAPI * LPWSPJOINLEAF)(
    __in SOCKET s,
    __in_bcount(namelen) const struct sockaddr FAR * name,
    __in int namelen,
    __in_opt LPWSABUF lpCallerData,
    __out_opt LPWSABUF lpCalleeData,
    __in_opt LPQOS lpSQOS,
    __in_opt LPQOS lpGQOS,
    __in DWORD dwFlags,
    __out LPINT lpErrno
    );

typedef
int
(WSPAPI * LPWSPLISTEN)(
    __in SOCKET s,
    __in int backlog,
    __out LPINT lpErrno
    );

typedef
int
(WSPAPI * LPWSPRECV)(
    __in SOCKET s,
    __in_ecount(dwBufferCount) LPWSABUF lpBuffers,
    __in DWORD dwBufferCount,
    __out_opt LPDWORD lpNumberOfBytesRecvd,
    __inout LPDWORD lpFlags,
    __inout_opt LPWSAOVERLAPPED lpOverlapped,
    __in_opt LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    __in_opt LPWSATHREADID lpThreadId,
    __in LPINT lpErrno
    );

typedef
int
(WSPAPI * LPWSPRECVDISCONNECT)(
    __in SOCKET s,
    __in_opt LPWSABUF lpInboundDisconnectData,
    __out LPINT lpErrno
    );

typedef
int
(WSPAPI * LPWSPRECVFROM)(
    __in SOCKET s,
    __in_ecount(dwBufferCount) LPWSABUF lpBuffers,
    __in DWORD dwBufferCount,
    __out_opt LPDWORD lpNumberOfBytesRecvd,
    __inout LPDWORD lpFlags,
    __out_bcount_part_opt(*lpFromLen, *lpFromLen) struct sockaddr FAR * lpFrom,
    __inout_opt LPINT lpFromlen,
    __inout_opt LPWSAOVERLAPPED lpOverlapped,
    __in_opt LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    __in_opt LPWSATHREADID lpThreadId,
    __out LPINT lpErrno
    );

typedef
int
(WSPAPI * LPWSPSELECT)(
    __in int nfds,
    __inout_opt fd_set FAR * readfds,
    __inout_opt fd_set FAR * writefds,
    __inout_opt fd_set FAR * exceptfds,
    __in_opt const struct timeval FAR * timeout,
    __out LPINT lpErrno
    );

typedef
int
(WSPAPI * LPWSPSEND)(
    __in SOCKET s,
    __in_ecount(dwBufferCount) LPWSABUF lpBuffers,
    __in DWORD dwBufferCount,
    __out_opt LPDWORD lpNumberOfBytesSent,
    __in DWORD dwFlags,
    __inout_opt LPWSAOVERLAPPED lpOverlapped,
    __in_opt LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    __in_opt LPWSATHREADID lpThreadId,
    __out LPINT lpErrno
    );

typedef
int
(WSPAPI * LPWSPSENDDISCONNECT)(
    __in SOCKET s,
    __in_opt LPWSABUF lpOutboundDisconnectData,
    __out LPINT lpErrno
    );

typedef
int
(WSPAPI * LPWSPSENDTO)(
    __in SOCKET s,
    __in_ecount(dwBufferCount) LPWSABUF lpBuffers,
    __in DWORD dwBufferCount,
    __out_opt LPDWORD lpNumberOfBytesSent,
    __in DWORD dwFlags,
    __in_bcount_opt(iToLen) const struct sockaddr FAR * lpTo,
    __in int iTolen,
    __inout_opt LPWSAOVERLAPPED lpOverlapped,
    __in_opt LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    __in_opt LPWSATHREADID lpThreadId,
    __out LPINT lpErrno
    );

typedef
int
(WSPAPI * LPWSPSETSOCKOPT)(
    __in SOCKET s,
    __in int level,
    __in int optname,
    __in_bcount_opt(optlen) const char FAR * optval,
    __in int optlen,
    __out LPINT lpErrno
    );

typedef
int
(WSPAPI * LPWSPSHUTDOWN)(
    __in SOCKET s,
    __in int how,
    __out LPINT lpErrno
    );

typedef
__checkReturn
SOCKET
(WSPAPI * LPWSPSOCKET)(
    __in int af,
    __in int type,
    __in int protocol,
    __in_opt LPWSAPROTOCOL_INFOW lpProtocolInfo,
    __in GROUP g,
    __in DWORD dwFlags,
    __out LPINT lpErrno
    );

typedef
INT
(WSPAPI * LPWSPSTRINGTOADDRESS)(
    __in LPWSTR AddressString,
    __in INT AddressFamily,
    __in_opt LPWSAPROTOCOL_INFOW lpProtocolInfo,
    __out_bcount_part(*lpAddressLength, *lpAddressLength) LPSOCKADDR lpAddress,
    __inout LPINT lpAddressLength,
    __out LPINT lpErrno
    );



typedef struct _WSPPROC_TABLE {

    LPWSPACCEPT              lpWSPAccept;
    LPWSPADDRESSTOSTRING     lpWSPAddressToString;
    LPWSPASYNCSELECT         lpWSPAsyncSelect;
    LPWSPBIND                lpWSPBind;
    LPWSPCANCELBLOCKINGCALL  lpWSPCancelBlockingCall;
    LPWSPCLEANUP             lpWSPCleanup;
    LPWSPCLOSESOCKET         lpWSPCloseSocket;
    LPWSPCONNECT             lpWSPConnect;
    LPWSPDUPLICATESOCKET     lpWSPDuplicateSocket;
    LPWSPENUMNETWORKEVENTS   lpWSPEnumNetworkEvents;
    LPWSPEVENTSELECT         lpWSPEventSelect;
    LPWSPGETOVERLAPPEDRESULT lpWSPGetOverlappedResult;
    LPWSPGETPEERNAME         lpWSPGetPeerName;
    LPWSPGETSOCKNAME         lpWSPGetSockName;
    LPWSPGETSOCKOPT          lpWSPGetSockOpt;
    LPWSPGETQOSBYNAME        lpWSPGetQOSByName;
    LPWSPIOCTL               lpWSPIoctl;
    LPWSPJOINLEAF            lpWSPJoinLeaf;
    LPWSPLISTEN              lpWSPListen;
    LPWSPRECV                lpWSPRecv;
    LPWSPRECVDISCONNECT      lpWSPRecvDisconnect;
    LPWSPRECVFROM            lpWSPRecvFrom;
    LPWSPSELECT              lpWSPSelect;
    LPWSPSEND                lpWSPSend;
    LPWSPSENDDISCONNECT      lpWSPSendDisconnect;
    LPWSPSENDTO              lpWSPSendTo;
    LPWSPSETSOCKOPT          lpWSPSetSockOpt;
    LPWSPSHUTDOWN            lpWSPShutdown;
    LPWSPSOCKET              lpWSPSocket;
    LPWSPSTRINGTOADDRESS     lpWSPStringToAddress;

} WSPPROC_TABLE, FAR * LPWSPPROC_TABLE;



typedef
BOOL
(WSPAPI * LPWPUCLOSEEVENT)(
    __in WSAEVENT hEvent,
    __out LPINT lpErrno
    );

typedef
int
(WSPAPI * LPWPUCLOSESOCKETHANDLE)(
    __in SOCKET s,
    __out LPINT lpErrno
    );

typedef
WSAEVENT
(WSPAPI * LPWPUCREATEEVENT)(
    __out LPINT lpErrno
    );

typedef
__checkReturn
SOCKET
(WSPAPI * LPWPUCREATESOCKETHANDLE)(
    __in DWORD dwCatalogEntryId,
    __in DWORD_PTR dwContext,
    __out LPINT lpErrno
    );

typedef
int
(WSPAPI * LPWPUFDISSET)(
    __in SOCKET s,
    __in fd_set FAR * fdset
    );

typedef
int
(WSPAPI * LPWPUGETPROVIDERPATH)(
    __in LPGUID lpProviderId,
    __out_ecount(*lpProviderDllPathLen) WCHAR FAR * lpszProviderDllPath,
    __inout LPINT lpProviderDllPathLen,
    __out LPINT lpErrno
    );

typedef
SOCKET
(WSPAPI * LPWPUMODIFYIFSHANDLE)(
    __in DWORD dwCatalogEntryId,
    __in SOCKET ProposedHandle,
    __out LPINT lpErrno
    );

typedef
BOOL
(WSPAPI * LPWPUPOSTMESSAGE)(
    __in HWND hWnd,
    __in UINT Msg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

typedef
int
(WSPAPI * LPWPUQUERYBLOCKINGCALLBACK)(
    __in DWORD dwCatalogEntryId,
    __out LPBLOCKINGCALLBACK FAR * lplpfnCallback,
    __out PDWORD_PTR lpdwContext,
    __out LPINT lpErrno
    );

typedef
int
(WSPAPI * LPWPUQUERYSOCKETHANDLECONTEXT)(
    __in SOCKET s,
    __out PDWORD_PTR lpContext,
    __out LPINT lpErrno
    );

typedef
int
(WSPAPI * LPWPUQUEUEAPC)(
    __in LPWSATHREADID lpThreadId,
    __in LPWSAUSERAPC lpfnUserApc,
    __in DWORD_PTR dwContext,
    __out LPINT lpErrno
    );

typedef
BOOL
(WSPAPI * LPWPURESETEVENT)(
    __in WSAEVENT hEvent,
    __out LPINT lpErrno
    );

typedef
BOOL
(WSPAPI * LPWPUSETEVENT)(
    __in WSAEVENT hEvent,
    __out LPINT lpErrno
    );

typedef
int
(WSPAPI * LPWPUOPENCURRENTTHREAD)(
    __out LPWSATHREADID lpThreadId,
    __out LPINT lpErrno
    );

typedef
int
(WSPAPI * LPWPUCLOSETHREAD)(
    __in LPWSATHREADID lpThreadId,
    __out LPINT lpErrno
    );



typedef
int
(WSPAPI * LPWPUCOMPLETEOVERLAPPEDREQUEST) (
    __in SOCKET s,
    __inout LPWSAOVERLAPPED lpOverlapped,
    __in DWORD dwError,
    __in DWORD cbTransferred,
    __out LPINT lpErrno
);

