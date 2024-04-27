/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-11-3
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mutex_utl.h"
#include "utl/exec_utl.h"
#include "utl/num_utl.h"
#include "utl/txt_utl.h"
#include "utl/atomic_once.h"
#include "utl/mem_utl.h"
#include "utl/list_dtq.h"
#include "mem_bs.h"
#include "mem_bs_func.h"

static MUTEX_S g_stMemLock;
static _MEM_LINES_S g_mem_count_level_ctrl[_MEM_MAX_LEVEL];

CONSTRUCTOR(init) {
    MUTEX_Init(&g_stMemLock);
}

#if 0
static void _mem_print_high_line_status()
{
    int i, j;
    int count;

    for (i=0; i<_MEM_MAX_LEVEL; i++) {
        count = 0;
        for (j=0; j<_MEM_LINE_HIGH_MAX; j++) {
            if (g_mem_count_level_ctrl[i].low_ctrl[j]) {
                count ++;
            }
        }
        printf(" level %u high_lins %u/%u \r\n", i, count, _MEM_LINE_HIGH_MAX);
    }
}
#endif

static inline _MEM_DESC_S * _mem_get_desc(int level, int line)
{
    int high_line = (line / _MEM_LINE_LOW_MAX) & (_MEM_LINE_HIGH_MAX -1);

    if (level >= _MEM_MAX_LEVEL) {
        return NULL;
    }

    _MEM_LINES_LOW_S *low_ctrl = g_mem_count_level_ctrl[level].low_ctrl[high_line];
    if (! low_ctrl) {
        return NULL;
    }

    int low_line = line & (_MEM_LINE_LOW_MAX - 1);

    return &low_ctrl->descs[low_line];
}

static inline int _mem_prepare_descs(int level, int line)
{
    _MEM_LINES_S *level_lines = &g_mem_count_level_ctrl[level];
    int high_line = (line / _MEM_LINE_LOW_MAX) & (_MEM_LINE_HIGH_MAX -1);

    if (level_lines->low_ctrl[high_line]) {
        return 0;
    }

    _MEM_Lock();
    if (! level_lines->low_ctrl[high_line]) {
        void *mem = malloc(sizeof(_MEM_LINES_LOW_S));
        memset(mem, 0, sizeof(_MEM_LINES_LOW_S));
        level_lines->low_ctrl[high_line] = mem;
    }
    _MEM_UnLock();

    if (level_lines->low_ctrl[high_line]) {
        return 0;
    }

    return BS_NO_MEMORY;
}

static UINT _mem_get_level_count(int level)
{
    int i;
    UINT count = 0;
    _MEM_DESC_S *desc;

    BS_DBGASSERT(level < _MEM_MAX_LEVEL);

    for (i=0; i<_MEM_LINE_MAX; i++) {
        desc = _mem_get_desc(level, i);
        if (desc) {
            count += desc->count;
        }
    }

    return count;
}

static void _mem_show_mem_stat_level(int level, char *file)
{
    int i;
    _MEM_DESC_S *desc;
    UINT count;
    char *filename;

    BS_DBGASSERT(level < _MEM_MAX_LEVEL);

    if (level < (_MEM_MAX_LEVEL - 1)) {
        EXEC_OutInfo(" Memory Size: %u \r\n", _mem_get_size_by_level(level));
    } else {
        EXEC_OutInfo(" Memory Size: >%u \r\n", _mem_get_size_by_level(level - 1));
    }

    EXEC_OutString(" Count   Filename(line) \r\n"
        "--------------------------------------------------------------------------\r\n");

    for (i=0; i<_MEM_LINE_MAX; i++) {
        desc = _mem_get_desc(level, i);
        if (! desc) {
            continue;
        }

        count = desc->count;
        filename = (void*)desc->filename;
        
        if ((! count) || (! filename)){
            continue;
        }

        if (file && (NULL == strstr(filename, file))) {
            continue;
        }

        EXEC_OutInfo(" %-7u %s(%d) \r\n", count, filename, i);
    }
}

static void _mem_show_mem_stat_all(char *file)
{
    int i;

    for (i=0; i<_MEM_MAX_LEVEL - 1; i++) {
        _mem_show_mem_stat_level(i, file);
        EXEC_OutString("\r\n");
    }

    _mem_show_mem_stat_level(i, file);
}

void _MEM_Lock()
{
    MUTEX_P(&g_stMemLock);
}

void _MEM_UnLock()
{
    MUTEX_V(&g_stMemLock);
}

PLUG_API void * MEM_MallocMem(ULONG size, const char *pszFileName, UINT ulLine)
{
    _MEM_HEAD_S *head;
    _MEM_TAIL_S *tail;
    int level = _mem_get_level_by_size(size);

    BS_DBGASSERT(ulLine < _MEM_LINE_MAX);
    BS_DBGASSERT(level < _MEM_MAX_LEVEL);

    if (level < _MEM_MAX_LEVEL) {
        if (_mem_prepare_descs(level, ulLine) != 0) {
            return NULL;
        }
    }

    head = malloc(size + sizeof(_MEM_HEAD_S) + sizeof(_MEM_TAIL_S));
    if (! head) {
        return NULL;
    }

    head->size = size;
    head->pszFileName = pszFileName;
    head->usLine = ulLine;
    head->level = level;
    head->busy = 1;
    head->uiHeadCheck = _MEM_DFT_CHECK_VALUE;

    char *mem = (void*)(head + 1);
    tail = (void*)(mem + size);
    tail->uiTailCheck = _MEM_DFT_CHECK_VALUE;

    _MEM_DESC_S *desc = _mem_get_desc(level, ulLine);
    if (desc) {
        desc->filename = pszFileName;
        ATOM_INC_FETCH(&desc->count);
    }

#ifdef IN_DEBUG
    _MEM_AddToDebugList(head);
#endif

    return mem;
}

PLUG_API void MEM_FreeMem(IN VOID *pMem, const char *pszFileName, IN UINT ulLine)
{
    _MEM_HEAD_S *head;
    _MEM_TAIL_S *tail;

    BS_DBGASSERT(NULL != pMem);

    head = _mem_get_head(pMem);
    tail = _mem_get_tail_by_head(head);

    if (! head->busy) {
        printf(" Mem Err: double free mem %s(%d),size:%d,caller:%s(%d)\r\n",
            head->pszFileName, head->usLine, head->size, pszFileName, ulLine);
        BS_DBGASSERT(0);
        abort();
    }

    BS_DBGASSERT(head->usLine < _MEM_LINE_MAX);

    if ((head->uiHeadCheck != _MEM_DFT_CHECK_VALUE)
            || (tail->uiTailCheck != _MEM_DFT_CHECK_VALUE)) {
        printf(" Mem Err: %s(%d),size:%d,level=%d,headcheck:0x%08x,tailcheck:0x%08x,caller:%s(%d)\r\n",
            head->pszFileName, head->usLine, head->size, head->level,
            head->uiHeadCheck, tail->uiTailCheck, pszFileName, ulLine);
        BS_DBGASSERT(0);
        abort();
    }

    _MEM_DESC_S *desc = _mem_get_desc(head->level, head->usLine);
    if (desc) {
        BS_DBGASSERT(desc->count > 0);
        ATOM_DEC_FETCH(&desc->count);
    }

#ifdef IN_DEBUG
    _MEM_RmFromList(head);
#endif

    head->busy = 0;
    head->uiHeadCheck = 0;
    tail->uiTailCheck = 0;

    free(head);
}


BOOL_T MEM_IsOverFlow(void *pMem)
{
    _MEM_HEAD_S *head = _mem_get_head(pMem);

    if (head->uiHeadCheck != _MEM_DFT_CHECK_VALUE) {
        BS_DBGASSERT(0);
        return TRUE;
    }

    _MEM_TAIL_S *tail = _mem_get_tail_by_head(head);
    if (tail->uiTailCheck != _MEM_DFT_CHECK_VALUE) {
        BS_DBGASSERT(0);
        return TRUE;
    }

    return FALSE;
}


BS_STATUS MEM_ShowStat(IN UINT ulArgc, IN CHAR **argv)
{
    UINT i;
    UINT count;

    EXEC_OutString(" Size         Count \r\n"
        "--------------------------------------------------------------------------\r\n");

    for (i=0; i<_MEM_MAX_LEVEL-1; i++) {
        count = _mem_get_level_count(i);
        EXEC_OutInfo(" %-11u  %-9u \r\n", _mem_get_size_by_level(i), count);
    }

    count = _mem_get_level_count(i);
    EXEC_OutInfo(" >%-10u  %-9u \r\n", _mem_get_size_by_level(i-1), count);

    return BS_OK;
}


BS_STATUS MEM_ShowSizeOfMemStat(int argc, char **argv)
{
    int level;
    char *file;

    level = _mem_cmd_get_show_level(argc, argv);
    file = _mem_cmd_get_show_file(argc, argv);

    if (level >= _MEM_MAX_LEVEL) {
        _mem_show_mem_stat_all(file);
    } else {
        _mem_show_mem_stat_level(level, file);
    }

    return BS_OK;
}

