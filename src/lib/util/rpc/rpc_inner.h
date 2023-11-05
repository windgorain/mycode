/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2009-4-13
* Description: 
* History:     
******************************************************************************/

#ifndef __RPC_INNER_H_
#define __RPC_INNER_H_

#ifdef __cplusplus
    extern "C" {
#endif 

#define _RPC_VER 1              
#define _RPC_ALIGN_MODE     4   



typedef struct
{
    UCHAR ucVer;              
    UCHAR ucMsgType;          
    UCHAR ucReturnType;       
    UCHAR ucReserved1;
    UINT ulTotalSize;        
    UINT ulParamNum;         
    UINT ulRpcHeadDataLen;
    UINT ulFirstParamOffset; 
}_RPC_DATA_HEAD_S;

typedef struct
{
    UCHAR ucType;            
    UCHAR ucReserved1;
    UCHAR ucReserved2;
    UCHAR ucReserved3;
    UINT ulParamLen;        
    UINT ulNextParamOffset; 
}_RPC_DATA_PARAM_NODE_S;

#ifdef __cplusplus
    }
#endif 

#endif 


