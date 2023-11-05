/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2014-2-25
* Description: 
* History:     
******************************************************************************/

#ifndef __VS_SOCKET_H_
#define __VS_SOCKET_H_

#ifdef __cplusplus
    extern "C" {
#endif 


#define SS_NOFDREF         0x0001    
#define SS_ISCONNECTED     0x0002    
#define SS_ISCONNECTING    0x0004    
#define SS_ISDISCONNECTING 0x0008    
#define SS_NBIO            0x0100    
#define SS_ISCONFIRMING    0x0400    
#define SS_CANBIND         0x1000    
#define SS_ISDISCONNECTED  0x2000    
#define SS_PROTOREF        0x4000    


typedef struct sockbuf
{

}VS_SOCKBUF_S;

typedef struct
{
    USHORT usType;
    USHORT usState;
    UINT   uiOptions;
    VS_FD_S *pstFile;
    VS_PROTOSW_S *pstProto;
    DLL_HEAD_S stCompList;
    DLL_HEAD_S stInCompList;
    VS_SOCKBUF_S stSndBuf;
    VS_SOCKBUF_S stRcvBuf;
}VS_SOCKET_S;


#ifdef __cplusplus
    }
#endif 

#endif 


