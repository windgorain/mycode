/*================================================================
*   Created by LiXingang: 2018.11.09
*   Description: 
*
================================================================*/
#ifndef _BUOY_UTL_H
#define _BUOY_UTL_H

#include "utl/bitmap_utl.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef unsigned int (*PF_BUOY_GET_BEST_ANCHOR_POINT)(void *user_data);

typedef struct {
    BITMAP_S bitmap;
    int buoys_num;
    int buoy_size; 
    void *buoys;
}BUOY_CTRL_S;

#ifdef __cplusplus
}
#endif
#endif 
