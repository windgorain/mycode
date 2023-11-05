/******************************************************************************
* Copyright (C),  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2011-3-24
* Description: 显示系统用到的Socket
* History:     
******************************************************************************/

#define RETCODE_FILE_NUM RETCODE_FILE_NUM_SOCKET_SHOW

#include "bs.h"

#include "utl/socket_utl.h"
#include "utl/bitmap1_utl.h"
#include "utl/txt_utl.h"
#include "utl/file_utl.h"
#include "utl/exec_utl.h"

#define _SSHOW_DFT_MAX_SOCKET_ID 8192

typedef struct
{
    BITMAP_S stBitMap;
}_SSHOW_CTRL_S;

typedef struct {
    const char *filename;
    int line;
}_SSHOW_FL_S;

static _SSHOW_FL_S g_sshow_fls[_SSHOW_DFT_MAX_SOCKET_ID];

#define _SSHOW_TYPE_TCP 0x1
#define _SSHOW_TYPE_UDP 0x2
#define _SSHOW_TYPE_RAW 0x4
#define _SSHOW_TYPE_OTHER 0x8
#define _SSHOW_TYPE_ALL 0xffffffff


STATIC _SSHOW_CTRL_S g_stSocketShowCtrl;

static void sshow_init()
{
    Mem_Zero(&g_stSocketShowCtrl, sizeof(_SSHOW_CTRL_S));
    BITMAP_Create(&g_stSocketShowCtrl.stBitMap, _SSHOW_DFT_MAX_SOCKET_ID);
}

CONSTRUCTOR(init) {
    sshow_init();
}

PLUG_API BS_STATUS _sshow_Add(IN INT iSocketId, const char *file, int line)
{
    if (iSocketId >= _SSHOW_DFT_MAX_SOCKET_ID)
    {
        RETURN(BS_OUT_OF_RANGE);
    }

    BS_DBGASSERT(NULL != file);

    BITMAP_SET(&g_stSocketShowCtrl.stBitMap, iSocketId);
    g_sshow_fls[iSocketId].filename = file;
    g_sshow_fls[iSocketId].line = line;

    return BS_OK;
}

PLUG_API VOID _sshow_Del(IN INT iSocketId)
{
    if (iSocketId >= _SSHOW_DFT_MAX_SOCKET_ID)
    {
        return;
    }

    BITMAP_CLR(&g_stSocketShowCtrl.stBitMap, iSocketId);

    return;
}

STATIC UINT sshow_GetTypeBitBySocketType(IN INT iType)
{
    UINT uiBit;

    switch (iType)
    {
        case SOCK_STREAM:
        {
            uiBit = _SSHOW_TYPE_TCP;
            break;
        }
        case SOCK_DGRAM:
        {
            uiBit = _SSHOW_TYPE_UDP;
            break;
        }
        case SOCK_RAW:
        {
            uiBit = _SSHOW_TYPE_RAW;
            break;
        }
        default:
        {
            uiBit = _SSHOW_TYPE_OTHER;
            break;
        }
    }
    
    return uiBit;
}

STATIC CHAR * sshow_GetTypeNameByType(IN INT iType)
{
    CHAR *pcTypeName = "";

    switch (iType)
    {
        case SOCK_STREAM:
        {
            pcTypeName = "TCP";
            break;
        }
        case SOCK_DGRAM:
        {
            pcTypeName = "UDP";
            break;
        }
        case SOCK_RAW:
        {
            pcTypeName = "RAW";
            break;
        }
        default:
        {
            pcTypeName = "UKN";
            break;
        }
    }
    return pcTypeName;
}

static VOID _sshow_Show (IN UINT uiTypeBit)
{
    UINT i;
    struct sockaddr_storage stSockAddrLocal;
    struct sockaddr_storage stSockAddrPeer;
    socklen_t iAddrLen;
    struct sockaddr_in *pstSinLocal;
    struct sockaddr_in *pstSinPeer;
    CHAR *pcFamily = "";
    USHORT usLocalPort;
    USHORT usPeerPort;
    UINT uiPeerIp;
    int iType;
    char *filename = "";

    EXEC_OutString(" ID    Type  Family  LPort  RIP              RPort  File\r\n"
        "------------------------------------------------------------------------------\r\n");

    for (i=1; i< _SSHOW_DFT_MAX_SOCKET_ID; i++)
    {
        if (BITMAP_ISSET(&g_stSocketShowCtrl.stBitMap, i))
        {
            iAddrLen = sizeof(int);
            if (0 != getsockopt(i, SOL_SOCKET, SO_TYPE, (CHAR*)&iType, &iAddrLen)) {

            }
            if ((uiTypeBit & sshow_GetTypeBitBySocketType(iType)) == 0) {
                continue;
            }

            usLocalPort = 0;
            usPeerPort = 0;
            uiPeerIp = 0;

            Mem_Zero(&stSockAddrLocal, sizeof(stSockAddrLocal));
            Mem_Zero(&stSockAddrPeer, sizeof(stSockAddrPeer));

            iAddrLen = sizeof(stSockAddrLocal);
            getsockname(i, (struct sockaddr *)&stSockAddrLocal, &iAddrLen);
            iAddrLen = sizeof(stSockAddrPeer);
            getpeername(i, (struct sockaddr *)&stSockAddrPeer, &iAddrLen);
            pstSinLocal = (struct sockaddr_in*)&stSockAddrLocal;

            switch (pstSinLocal->sin_family)
            {
                case AF_INET:
                {
                    pstSinPeer = (struct sockaddr_in*)&stSockAddrPeer;
                    pcFamily = "IPv4";
                    usLocalPort = pstSinLocal->sin_port;
                    usPeerPort = pstSinPeer->sin_port;
                    uiPeerIp = pstSinPeer->sin_addr.s_addr;
                    break;
                }
                case AF_INET6:
                {
                    pcFamily = "IPv6";
                    break;
                }
                case AF_UNIX:
                {
                    pcFamily = "UINX";
                    break;
                }
                default:
                {
                    pcFamily = "N/A";
                    break;
                }
            }

            usLocalPort = ntohs(usLocalPort);
            usPeerPort = ntohs(usPeerPort);
            uiPeerIp = ntohl(uiPeerIp);

            filename = "";
            if (g_sshow_fls[i].filename) {
                filename = FILE_GetFileNameFromPath((char*)g_sshow_fls[i].filename);
            }

            EXEC_OutInfo(" %-4d  %-4s  %-6s  %-5d  %-15s  %-5d  %s:%d \r\n",
                i, sshow_GetTypeNameByType(iType), pcFamily, 
                usLocalPort, Socket_IpToName(uiPeerIp), usPeerPort,
                filename, g_sshow_fls[i].line);
        }
    }
    EXEC_OutString("\r\n");
}

BS_STATUS SSHOW_ShowAll (IN UINT ulArgc, IN CHAR **argv)
{
    _sshow_Show(_SSHOW_TYPE_ALL);
    return BS_OK;
}

BS_STATUS SSHOW_ShowTcp (IN UINT ulArgc, IN CHAR **argv)
{
    _sshow_Show(_SSHOW_TYPE_TCP);
    return BS_OK;
}

BS_STATUS SSHOW_ShowUdp (IN UINT ulArgc, IN CHAR **argv)
{
    _sshow_Show(_SSHOW_TYPE_UDP);
    return BS_OK;
}


