/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-11-20
* Description: 
* History:     
******************************************************************************/

#ifndef __UIPC_DEF_H_
#define __UIPC_DEF_H_

#ifdef __cplusplus
    extern "C" {
#endif 

#define M_DONTWAIT  0
#define M_TRYWAIT   1
#define M_WAIT      2


enum sopt_dir { SOPT_GET, SOPT_SET };
typedef struct sockopt
{
    enum   sopt_dir sopt_dir; 
    int    sopt_level;    
    int    sopt_name;    
    void  *sopt_val;    
    int   sopt_valsize;    
}SOCKOPT_S;

typedef struct keep_alive_arg
{
    short ka_idle;     
    short ka_intval;   
    short ka_count;    
}KEEP_ALIVE_ARG_S;





#ifdef __cplusplus
    }
#endif 

#endif 


