/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _RP_UTL_H
#define _RP_UTL_H
#include "utl/mime_utl.h"
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    char tag_start; /* 关键字起始标记 */
    char tag_stop; /* 关键字结束标记 */
}RP_TAG_S;

int RP_Do(RP_TAG_S *tag, MIME_HANDLE kv, char *in_str, char *out_str, int out_str_size);

#ifdef __cplusplus
}
#endif
#endif //RP_UTL_H_
