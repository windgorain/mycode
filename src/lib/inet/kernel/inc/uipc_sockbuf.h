/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-11-21
* Description: 
* History:     
******************************************************************************/

#ifndef __UIPC_SOCKBUF_H_
#define __UIPC_SOCKBUF_H_

#ifdef __cplusplus
    extern "C" {
#endif 


#define    SBL_WAIT    0x00000001    
#define    SBL_NOINTR    0x00000002    
#define    SBL_VALID    (SBL_WAIT | SBL_NOINTR)


#define    SB_WAIT      0x04        
#define    SB_SEL       0x08        
#define    SB_ASYNC     0x10        
#define    SB_UPCALL    0x20        
#define    SB_NOINTR    0x40        
#define    SB_AIO       0x80        
#define    SB_KNOTE     0x100       
#define    SB_AUTOSIZE  0x800       

#define SB_EMPTY_FIXUP(sb) do { \
    if ((sb)->sb_mb == NULL) { \
        (sb)->sb_lastrecord = NULL; \
    } \
} while (0)

#define    sbfree(sb, len) { \
    (sb)->sb_cc -= len; \
}

#define    sballoc(sb, len) { \
    (sb)->sb_cc += len; \
}

#define    sbspace(sb) \
    (int)((sb)->sb_hiwat - (sb)->sb_cc)

typedef struct sockbuf
{
    SEM_HANDLE sb_wait;
    SEM_HANDLE sb_mtx;
    MUTEX_S sb_sx;        
    MBUF_S *sb_mb;    
    MBUF_S *sb_lastrecord;
    UINT sb_cc;
    UINT sb_hiwat;
    UINT sb_lowat;
    INT  sb_timeo;      
    USHORT sb_state;    
    USHORT sb_flags;
}SOCKBUF_S;

#define MA_NOTOWNED 0x00000000
#define MA_OWNED    0x00000001
#define SOCKBUF_MTX(_sb)        ((_sb)->sb_mtx)
#define SOCKBUF_LOCK(_sb)        SEM_P(SOCKBUF_MTX(_sb), BS_WAIT, BS_WAIT_FOREVER)
#define SOCKBUF_UNLOCK(_sb)      SEM_V(SOCKBUF_MTX(_sb))
#define SOCKBUF_LOCK_ASSERT(_sb)
#define mtx_assert(_mutex, _what)
#define SBLASTRECORDCHK(_sb)


#ifdef __cplusplus
    }
#endif 

#endif 


