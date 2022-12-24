/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _LDATA_UTL_H
#define _LDATA_UTL_H
#ifdef __cplusplus
extern "C"
{
#endif

/* 消除头字符 */
void LDATA_StrimHead(LDATA_S *data, LDATA_S *rm_chars, OUT LDATA_S *out_data);
/* 消除尾字符 */
void LDATA_StrimTail(LDATA_S *data, LDATA_S *rm_chars, OUT LDATA_S *out_data);
/* 消除头尾的字符 */
void LDATA_Strim(LDATA_S *data, char *rm_chars, OUT LDATA_S *out_data);
/* 查找字符 */
void * LDATA_Strchr(LDATA_S *data, UCHAR ch);
/* 反向查找字符 */
void * LDATA_ReverseChr(LDATA_S *data, UCHAR ch);
/* 查找数据 */
void * LDATA_Strstr(LDATA_S *data, LDATA_S *to_find);
/* 忽略大小写查找 */
void * LDATA_Stristr(LDATA_S *data, LDATA_S *to_find);
/* 将数据分隔成两个 */
void LDATA_MSplit(LDATA_S *in, LDATA_S *split_chars, OUT LDATA_S *out1, OUT LDATA_S *out2);
/* 将数据分隔成两个 */
void LDATA_Split(LDATA_S *in, UCHAR split_char, OUT LDATA_S *out1, OUT LDATA_S *out2);
/* 将数据分隔成多个, 返回分割成了多少个 */
UINT LDATA_XSplit(LDATA_S *in, UCHAR split_char, OUT LDATA_S *outs, UINT count);
/* 数据比较 */
int LDATA_Cmp(LDATA_S *data1, LDATA_S *data2);
/* 忽略大小写比较 */
INT LDATA_CaseCmp(LDATA_S *data1, LDATA_S *data2);

#ifdef __cplusplus
}
#endif
#endif //LDATA_UTL_H_
