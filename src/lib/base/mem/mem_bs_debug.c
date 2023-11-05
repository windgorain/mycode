#include "bs.h"

#include "utl/mutex_utl.h"
#include "utl/exec_utl.h"
#include "utl/num_utl.h"
#include "utl/txt_utl.h"
#include "utl/atomic_once.h"
#include "utl/mem_utl.h"
#include "utl/list_dtq.h"
#include "mem_bs.h"

#ifdef IN_DEBUG

typedef struct {
    DLL_NODE_S stLinkNode;
    _MEM_HEAD_S mem_info;
    UINT ulCount;
}_MEM_FILE_COUNT_S;

static DTQ_HEAD_S g_mem_level_mem_list[_MEM_MAX_LEVEL];

static void _mem_check_list(IN UINT uiLevel)
{
    DTQ_NODE_S *node;
    _MEM_HEAD_S *head;
    _MEM_TAIL_S *tail;

    DTQ_FOREACH(&g_mem_level_mem_list[uiLevel], node) {
        head = container_of(node, _MEM_HEAD_S, link_node);
        tail = _mem_get_tail_by_head(head);
        if ((head->uiHeadCheck != _MEM_DFT_CHECK_VALUE)
            || (tail->uiTailCheck != _MEM_DFT_CHECK_VALUE)
            || (head->level >= _MEM_MAX_LEVEL)
            || (head->busy == 0)) {
            printf(" Mem Err: %s(%d): size:%d,HeadCheck:0x%08x,TailCheck:0x%08x \r\n",
                head->pszFileName, head->usLine, head->size,
                head->uiHeadCheck, tail->uiTailCheck);
            BS_DBGASSERT(0);
        }
    }

}

static VOID _mem_check_mem(IN HANDLE hTimeHandle, IN USER_HANDLE_S *pstUserHandle)
{
    static UINT ulLevel = 0;

    _MEM_Lock();
    
    _mem_check_list(ulLevel);

    ulLevel ++;

    if (ulLevel >= _MEM_MAX_LEVEL) {
        ulLevel = 0;
    }

    _MEM_UnLock();
}

static int _mem_init_once(void *ud)
{
    int i;

    for (i=0; i<_MEM_MAX_LEVEL; i++) {
        DTQ_Init(&g_mem_level_mem_list[i]);
    }

    return 0;
}

static void _mem_init()
{
    static ATOM_ONCE_S once = ATOM_ONCE_INIT_VALUE;
    AtomOnce_WaitDo(&once, _mem_init_once, NULL);
}

static int _mem_file_line_cmp(DLL_NODE_S *pstNode1, DLL_NODE_S *pstNode2, HANDLE hUserHandle)
{
    _MEM_FILE_COUNT_S *node1 = (void*)pstNode1;
    _MEM_FILE_COUNT_S *node2 = (void*)pstNode2;

    int ret = strcmp(node1->mem_info.pszFileName, node2->mem_info.pszFileName);
    if (ret != 0) {
        return ret;
    }

    ret = (int)(node1->mem_info.usLine - node2->mem_info.usLine);
    if (ret != 0) {
        return ret;
    }

    return (int)(node1->mem_info.size - node2->mem_info.size);
}

static int _mem_line_cmp(DLL_NODE_S *pstNode1, DLL_NODE_S *pstNode2, HANDLE hUserHandle)
{
    _MEM_FILE_COUNT_S *node1 = (void*)pstNode1;
    _MEM_FILE_COUNT_S *node2 = (void*)pstNode2;

    return (int)(node1->mem_info.usLine - node2->mem_info.usLine);
}


static void _mem_count_by_file(IN DLL_HEAD_S *pstDllHead, IN _MEM_HEAD_S *pstMemHead)
{
    _MEM_FILE_COUNT_S *pstNode;

    
    DLL_SCAN(pstDllHead, pstNode) {
        if ((pstNode->mem_info.usLine == pstMemHead->usLine)
                && (pstNode->mem_info.size == pstMemHead->size)
                && (0 == strcmp(pstNode->mem_info.pszFileName, pstMemHead->pszFileName))) {
            pstNode->ulCount ++;
            return;
        }
    }

    
    pstNode = malloc(sizeof(_MEM_FILE_COUNT_S));    
    if (NULL == pstNode) {
        return;
    }

    memset(pstNode, 0, sizeof(_MEM_FILE_COUNT_S));
    memcpy(&pstNode->mem_info, pstMemHead, sizeof(_MEM_HEAD_S));
    pstNode->ulCount = 1;

    DLL_SortAdd(pstDllHead, &pstNode->stLinkNode, _mem_file_line_cmp, NULL);
}


static void _mem_count_line_conflict(IN DLL_HEAD_S *pstDllHead, IN _MEM_HEAD_S *pstMemHead)
{
    _MEM_FILE_COUNT_S *pstNode;
    int conflict = 0;

    
    DLL_SCAN(pstDllHead, pstNode) {
        if (pstNode->mem_info.usLine == pstMemHead->usLine) {
            if (0 == strcmp(pstNode->mem_info.pszFileName, pstMemHead->pszFileName)) {
                return;
            } else {
                conflict = 1;
                pstNode->ulCount = 1;
            }
        }
    }

    
    pstNode = malloc(sizeof(_MEM_FILE_COUNT_S));    
    if (NULL == pstNode) {
        return;
    }

    memset(pstNode, 0, sizeof(_MEM_FILE_COUNT_S));
    memcpy(&pstNode->mem_info, pstMemHead, sizeof(_MEM_HEAD_S));
    pstNode->ulCount = conflict;

    DLL_SortAdd(pstDllHead, &pstNode->stLinkNode, _mem_line_cmp, NULL);
}

static void _mem_free_count_file(IN DLL_HEAD_S *pstDllHead)
{
    _MEM_FILE_COUNT_S *pstNode, *pstNodeTmp;

    DLL_SAFE_SCAN(pstDllHead,pstNode,pstNodeTmp) {
        DLL_DEL(pstDllHead, pstNode);
        free(pstNode);
    }
}

static void _mem_debug_show_level_mem(int level, char *file)
{
    DTQ_NODE_S *link_node;
    _MEM_HEAD_S *pstHead;
    DLL_HEAD_S stDllHead;
    _MEM_FILE_COUNT_S *pstNode;

    if (level >= _MEM_MAX_LEVEL) {
        return;
    }

    DLL_INIT(&stDllHead);

    _MEM_Lock();
    DTQ_FOREACH(&g_mem_level_mem_list[level], link_node) {
        pstHead = container_of(link_node, _MEM_HEAD_S, link_node);
        if (file && (NULL == strstr(pstHead->pszFileName, file))) {
            continue;
        }
        _mem_count_by_file(&stDllHead, pstHead); 
    }
    _MEM_UnLock();

    EXEC_OutString(" Count   Size      Filename(line)\r\n"
        "--------------------------------------------------------------------------\r\n");

    
    DLL_SCAN(&stDllHead, pstNode) {
        EXEC_OutInfo(" %-7d %-9d %s(%d)\r\n",
                pstNode->ulCount, pstNode->mem_info.size, pstNode->mem_info.pszFileName, pstNode->mem_info.usLine);
    }

    EXEC_OutString("\r\n");

    _mem_free_count_file(&stDllHead);
}

static void _mem_debug_show_line_conflict(int level)
{
    DTQ_NODE_S *link_node;
    _MEM_HEAD_S *pstHead;
    DLL_HEAD_S stDllHead;
    _MEM_FILE_COUNT_S *pstNode;

    if (level >= _MEM_MAX_LEVEL) {
        return;
    }

    DLL_INIT(&stDllHead);

    _MEM_Lock();
    DTQ_FOREACH(&g_mem_level_mem_list[level], link_node) {
        pstHead = container_of(link_node, _MEM_HEAD_S, link_node);
        _mem_count_line_conflict(&stDllHead, pstHead); 
    }
    _MEM_UnLock();

    
    DLL_SCAN(&stDllHead, pstNode) {
        if (pstNode->ulCount > 0) {
            EXEC_OutInfo(" %s(%d)\r\n", pstNode->mem_info.pszFileName, pstNode->mem_info.usLine);
        }
    }

    EXEC_OutString("\r\n");

    _mem_free_count_file(&stDllHead);
}

static void _mem_debug_show_mem_all(char *file)
{
    int i;

    for (i=0; i<_MEM_MAX_LEVEL - 1; i++) {
        EXEC_OutInfo(" Size: %u \r\n", _mem_get_size_by_level(i));
        _mem_debug_show_level_mem(i, file);
        EXEC_OutString("\r\n");
    }

    EXEC_OutInfo(" Size: >%u \r\n", _mem_get_size_by_level(i-1));
    _mem_debug_show_level_mem(i, file);
}

void _MEM_AddToDebugList(_MEM_HEAD_S *head)
{
    _mem_init();

    _MEM_Lock();
    DTQ_AddTail(&g_mem_level_mem_list[head->level], &head->link_node);
    _MEM_UnLock();
}

void _MEM_RmFromList(_MEM_HEAD_S *head)
{
    _MEM_Lock();
    DTQ_Del(&head->link_node);
    _MEM_UnLock();
}


BS_STATUS MemDebug_ShowSizeOfMem(int argc, char **argv)
{
    int level;
    char *file;

    level = _mem_cmd_get_show_level(argc, argv);
    file = _mem_cmd_get_show_file(argc, argv);

    if (level >= _MEM_MAX_LEVEL) {
        _mem_debug_show_mem_all(file);
    } else {
        _mem_debug_show_level_mem(level, file);
    }

    return 0;
}


BS_STATUS MemDebug_ShowLineConflict(int argc, char **argv)
{
    int i;

    for (i=0; i<_MEM_MAX_LEVEL - 1; i++) {
        EXEC_OutInfo(" Size: %u \r\n", _mem_get_size_by_level(i));
        _mem_debug_show_line_conflict(i);
        EXEC_OutString("\r\n");
    }

    EXEC_OutInfo(" Size: >%u \r\n", _mem_get_size_by_level(i-1));
    _mem_debug_show_line_conflict(i);

    return 0;
}


int MemDebug_Check(int argc, char **argv)
{
    int ret;
    static MTIMER_S g_stMemMTimer;

    
    ret = MTimer_Add(&g_stMemMTimer, 1000, TIMER_FLAG_CYCLE, _mem_check_mem, NULL);
    if (ret < 0) {
        BS_DBGASSERT(0);
        RETURN(BS_ERR);
    }

    return 0;
}


#endif
