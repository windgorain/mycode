/*================================================================
*   Created by LiXingang
*   Description: 所字符串扫描引擎: m p scan engine
*
================================================================*/
#ifndef _MPSE_ENGINE_H
#define _MPSE_ENGINE_H
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    DLL_NODE_S link_node;
    char *pattern;
    int rule_id;
    UCHAR nocase; /* 指定对content字符串大小写不敏感 */
    UCHAR pattern_index; /* 对应第几个pattern */
    int distance; /* content选项的修饰符，设定模式匹配间的最小间距 */
    int within;  /* content选项的修饰符，设定模式匹配间的最大间距 */
    int offset; /* content选项的修饰符，设定开始搜索的位置 */
    int depth; /* content选项的修饰符，设定搜索的最大深度 */
}MPSE_PATTERN_S;

typedef void* MPSE_HANDLE;

MPSE_HANDLE MPSE_New();
void MPSE_Free(MPSE_HANDLE hMpse);

int MPSE_AddPattern(MPSE_HANDLE hMpse, MPSE_PATTERN_S *pattern);
int MPSE_Compile(MPSE_HANDLE hMpse);
int MPSE_Match(MPSE_HANDLE hMpse, UCHAR *data, int data_len);

#ifdef __cplusplus
}
#endif
#endif //MPSE_ENGINE_H_
