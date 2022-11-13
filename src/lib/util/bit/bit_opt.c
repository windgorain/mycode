/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/bit_opt.h"

/* 获取最低位的index, 比如 0x1返回0, 0x2返回1, 0x4返回2 */
int BIT_GetLastIndex(UINT num)
{
    int i;

    for (i=0; i<32; i++) {
        if (num & (1 << i)) { 
            return i;
        }
    }

    return -1;
}

/* 获取最高位的index */
int BIT_GetFirstIndex(UINT num)
{
    int i;

    for (i=31; i>=0; i--) {
        if (num & (1 << i)) { 
            return i;
        }
    }

    return -1;
}

/* sprint bit位 */
char * BIT_SPrint(uint32_t v, uint32_t off, uint32_t size, OUT char *buf)
{
    uint32_t tmp = BIT_GET_OFF(v, off, size);
    return TXT_Num2BitString(tmp, size, buf);
}

/* print bit位 */
void BIT_Print(uint32_t v, uint32_t off, uint32_t size, PF_PRINT_FUNC func)
{
    char info[65];
    func("%s", BIT_SPrint(v, off, size, info));
}

/* sprint多个名字和bit位
   出错返回 <0 的值
   >=0 返回构建后的字符串长度
 */
int BIT_XSPrint(uint32_t v, BIT_DESC_S *desc, int desc_num, OUT char *buf, uint32_t buf_size)
{
    int i;
    char *out = buf;
    uint32_t size = buf_size;
    int len;
    char info[65];

    for (i=0; i<desc_num; i++) {
        len = snprintf(out, size, "|%s", BIT_SPrint(v, desc[i].off, desc[i].size, info));
        if (len >= size) {
            RETURN(BS_OUT_OF_RANGE);
        }
        out += len;
        size -= len;
    }

    return 0;
}


