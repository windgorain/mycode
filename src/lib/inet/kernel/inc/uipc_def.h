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
#endif /* __cplusplus */

#define M_DONTWAIT  0
#define M_TRYWAIT   1
#define M_WAIT      2


enum sopt_dir { SOPT_GET, SOPT_SET };
typedef struct sockopt
{
    enum   sopt_dir sopt_dir; /* is this a get or a set? */
    int    sopt_level;    /* second arg of [gs]etsockopt */
    int    sopt_name;    /* third arg of [gs]etsockopt */
    void  *sopt_val;    /* fourth arg of [gs]etsockopt */
    int   sopt_valsize;    /* (almost) fifth arg of [gs]etsockopt */
}SOCKOPT_S;

typedef struct keep_alive_arg
{
    short ka_idle;     /* idle time before keep alive probe, second */
    short ka_intval;   /* probe interval time, second */
    short ka_count;    /* probe count */
}KEEP_ALIVE_ARG_S;

/* Bits in the FLAGS argument to `send', `recv', et al.  */



#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__UIPC_DEF_H_*/


