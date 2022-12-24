/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _QSORT_UTL_H
#define _QSORT_UTL_H
#ifdef __cplusplus
extern "C"
{
#endif

void QSORT_Do(void *base, int count, int ele_size, PF_CMP_FUNC cmp_func, void *ud);

#ifdef __cplusplus
}
#endif
#endif //QSORT_UTL_H_
