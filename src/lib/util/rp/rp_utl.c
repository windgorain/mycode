/*================================================================
*   Created by LiXingang
*   Description: 字符串替换
*    eg: show <name> : 其中<name>就是要替换的关键字
*
================================================================*/
#include "bs.h"
#include "utl/rp_utl.h"
#include "utl/txt_utl.h"
#include "utl/mime_utl.h"

#define RP_KEY_MAX_LEN 63


/*
   替换后的结果放到out_str中,并且最后补0
   如果失败,返回值<0，成功则返回生成结果的字符串长度
 */
int RP_Do(RP_TAG_S *tag, MIME_HANDLE kv, char *in_str, char *out_str, int out_str_size)
{
    char *tag_start, *key_start;
    char *tag_stop;
    char *in = in_str;
    char *out = out_str;
    int size = out_str_size;
    int cpy_len;
    char *value;
    int key_len;
    char key[RP_KEY_MAX_LEN + 1];

    while(*in != '\0') {
        tag_start = strchr(in, tag->tag_start);
        if (! tag_start) {
            cpy_len = strlcpy(out, in, size);
            if (cpy_len >= size) {
                RETURN(BS_OUT_OF_RANGE);
            }
            size -= cpy_len;
            break;
        }
        key_start = tag_start + 1;

        tag_stop = strchr(key_start, tag->tag_stop);
        if (! tag_stop) {
            RETURN(BS_ERR);
        }
        in = tag_stop + 1;

        key_len = tag_stop - key_start;
        if (key_len == 0) {
            continue;
        }

        if (key_len > RP_KEY_MAX_LEN) {
            RETURN(BS_NOT_SUPPORT);
        }

        memcpy(key, key_start, key_len);
        key[key_len] = '\0';

        value = MIME_GetKeyValue(kv, key);
        if (value) {
            cpy_len = strlcpy(out, value, size);
            if (cpy_len >= size) {
                RETURN(BS_OUT_OF_RANGE);
            }
            size -= cpy_len;
        }
    }

    cpy_len = out_str_size - size;

    return cpy_len;
}


