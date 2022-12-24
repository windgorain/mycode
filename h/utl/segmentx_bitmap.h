/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _SEGMENTX_BITMAP_H
#define _SEGMENTX_BITMAP_H
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    SBITMAP_S sbitmap;
    UINT bitsize; /* 支持的bit个数, 为32的整数倍 */ 
}SXBITMAP_S;


int SXBitmap_Init(SXBITMAP_S *ctrl, UINT bitsize);
void SXBitmap_Finit(SXBITMAP_S *ctrl);
int SXBitmap_Set(SXBITMAP_S *ctrl, UINT index);
void SXBitmap_Clr(SXBITMAP_S *ctrl, UINT index);



#ifdef __cplusplus
}
#endif
#endif 
