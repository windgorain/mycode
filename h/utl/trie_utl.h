/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _TRIE_UTL_H
#define _TRIE_UTL_H
#ifdef __cplusplus
extern "C"
{
#endif

/* 低16位预定义标记, 用户自定义标记位高16位 */
#define TRIE_NODE_FLAG_MATCHED          0x1 /* 匹配节点*/
#define TRIE_NODE_FLAG_WILDCARD         0x2 /* 通配, 比如"*.com" , 则insert时传入".com"和此标记 */

typedef HANDLE TRIE_HANDLE;

enum {
    TRIE_MATCH_EXACT=0,  /* 精确匹配,忽略通配标志 */
    TRIE_MATCH_MAXLEN,   /* 最长匹配,忽略通配标志,比如:abcd可以匹配abc和ab,但会返回匹配的最长的abc */
    TRIE_MATCH_WILDCARD, /* 最长通配匹配,比如abcd可以匹配abcd,abc*,ab*,但不会匹配ab,abcd为最高优先级,abc*其次,ab*再次 */
    TRIE_MATCH_HOSTNAME, /* 最长通配域名格式,比如www.baidu.com可以匹配baidu.com */

    TRIE_MATCH_INVALID
};

enum {
    TRIE_TYPE_SIMPLE = 0, /* 最简单的256叉树 */
    TRIE_TYPE_LL,     /* 使用链表代替256叉树 */
    TRIE_TYPE_DARRAY, /* 动态数组代替256叉树 */
    TRIE_TYPE_4BITS,  /* 16叉树, 一个字节拆成两个4bits */
    TRIE_TYPE_2BITS,  /* 4叉树, 一个字节拆成四个2bits */
};

typedef struct {
    void *ud;
    UINT flag;
}TRIE_COMMON_S;

typedef void (*PF_TRIE_UD_FREE)(void *ud);
typedef void* (*PF_TRIE_UD_SET)(void *ud, void *uh);

/* old为上次确定的节点; cur为当前匹配的节点;返回值为本次确定的节点 */
typedef TRIE_COMMON_S* (*PF_TRIE_MATCH_CB)(TRIE_COMMON_S *old, TRIE_COMMON_S *cur, int matched_len, void *ud);


TRIE_HANDLE Trie_Create(int type);
void Trie_Destroy(TRIE_HANDLE trie_handle, PF_TRIE_UD_FREE free_ud);
TRIE_COMMON_S * Trie_InsertEx(TRIE_HANDLE trie_handle, UCHAR *data, int data_len, UINT flag,
        PF_TRIE_UD_SET ud_set, void * uh);
TRIE_COMMON_S * Trie_Insert(TRIE_HANDLE trie_handle, UCHAR *data, int data_len, UINT flag, void * ud);
/* 返回ud */
void * Trie_Del(TRIE_HANDLE trie_handle, UCHAR *data, int data_len);
TRIE_COMMON_S * Trie_PrefixMatch(TRIE_HANDLE trie_handle, UCHAR *data, int data_len, PF_TRIE_MATCH_CB func, void *ud);
TRIE_COMMON_S * Trie_Match(TRIE_HANDLE trie_handle, UCHAR *data, int data_len, int match_type);

#ifdef __cplusplus
}
#endif
#endif //TRIE_UTL_H_
