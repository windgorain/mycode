/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _MEM_BS_H
#define _MEM_BS_H
#ifdef __cplusplus
extern "C"
{
#endif

#define _MEM_LINE_MAX 4096 /* 最大行数, 必须为2的指数幂 */
#define _MEM_LINE_LOW_MAX 32
#define _MEM_LINE_HIGH_MAX (_MEM_LINE_MAX / _MEM_LINE_LOW_MAX)
#define _MEM_MAX_LEVEL 9

#define _MEM_DFT_CHECK_VALUE 0x0a0b0c0d

#define _MEM_GET_SPLIT_MEM_USRSIZE(ulLevel)    (32<<(ulLevel))

typedef struct {
#ifdef IN_DEBUG
    DTQ_NODE_S link_node;
#endif
    const char *pszFileName;
    UINT usLine:16;
    UINT level:4;
    UINT busy:1;
    UINT size;          /* 用户真实申请的内存大小 */
    UINT uiHeadCheck;
}_MEM_HEAD_S;

typedef struct {
    UINT       uiTailCheck;
}_MEM_TAIL_S;

typedef struct {
    volatile UINT count;
    const char *filename;
}_MEM_DESC_S;

typedef struct {
    _MEM_DESC_S descs[_MEM_LINE_LOW_MAX];
}_MEM_LINES_LOW_S;

typedef struct {
    _MEM_LINES_LOW_S *low_ctrl[_MEM_LINE_HIGH_MAX];
}_MEM_LINES_S;

static inline _MEM_HEAD_S * _mem_get_head(void *pMem)
{
    return (void*)(((char*)pMem) - sizeof(_MEM_HEAD_S));
}

static inline _MEM_TAIL_S * _mem_get_tail_by_head(_MEM_HEAD_S *head)
{
    return (void*)((char*)(head + 1) + head->size);
}

/* 根据大小得到Level */
static inline UINT _mem_get_level_by_size(IN ULONG size)
{
    int level;

    for (level=0; level<(_MEM_MAX_LEVEL-1); level++) {
        if (size <= (32 << level)) {
            break;
        }
    }

    return level;
}

static inline int _mem_cmd_get_show_level(int argc, char **argv)
{
    int level;
    int size;

    if (argv[3][0] == 'a') {
        level = _MEM_MAX_LEVEL;
    } else if (argv[3][0] == 'l') {
        level = _MEM_MAX_LEVEL - 1;
    } else {
        size = TXT_Str2Ui(argv[3]);
        level = _mem_get_level_by_size(size);
    }

    return level;
}

static inline char * _mem_cmd_get_show_file(int argc, char **argv)
{
    if (argc < 6) {
        return NULL;
    }

    if (strcmp(argv[4], "file") != 0) {
        return NULL;
    }

    return argv[5];
}

void _MEM_Lock();
void _MEM_UnLock();

#ifdef __cplusplus
}
#endif
#endif //MEM_BS_H_
