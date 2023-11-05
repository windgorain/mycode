/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _IPMASK_BITMAP_H
#define _IPMASK_BITMAP_H
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    UCHAR depth; 
    UCHAR *bits;
}IPMASK_BITMAP_S;

#define IPMASK_BITMAP_DEPTH_2_BYTES(depth) (PREFIX_2_COUNT(depth) >> 3)

void IPMASK_BITMAP_Init(IPMASK_BITMAP_S *ctrl, UCHAR depth);
int IPMASK_BITMAP_Add(IPMASK_BITMAP_S *ctrl, UINT ip, UINT mask);
int IPMASK_BITMAP_Del(IPMASK_BITMAP_S *ctrl, UINT ip, UINT mask);
int IPMASK_BITMAP_IsSet(IPMASK_BITMAP_S *ctrl, UINT ip);
void IPMASK_BITMAP_Show(IPMASK_BITMAP_S *ctrl);

#ifdef __cplusplus
}
#endif
#endif 
