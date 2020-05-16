/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/bitmap_utl.h"
#include "utl/match_utl.h"

typedef struct {
    UINT64 match_count;
    void *pattern;
    void *ud;
}MATCH_NODE_S;

typedef struct {
    int max;
    int pattern_size;
    PF_MATCH_COUNT_CMP pfcmp;
    BITMAP_S bitmap;
    MATCH_NODE_S nodes[0];
}MATCH_S;

MATCH_HANDLE Match_Create(int max, int pattern_size,
        PF_MATCH_COUNT_CMP pfcmp)
{
    int i;
    MATCH_S *ctrl;
    int size = sizeof(MATCH_S) + (max * pattern_size)
        + (max * sizeof(MATCH_NODE_S));

    ctrl = MEM_ZMalloc(size);
    if (ctrl == NULL) {
        return NULL;
    }

    if (0 != BITMAP_Create(&ctrl->bitmap, max)) {
        MEM_Free(ctrl);
        return NULL;
    }

    char *pattern = (void*)&ctrl->nodes[max];
    for (i=0; i<max; i++) {
        ctrl->nodes[i].pattern = pattern;
        pattern += pattern_size;
    }

    ctrl->max = max;
    ctrl->pfcmp = pfcmp;
    ctrl->pattern_size = pattern_size;

    return ctrl;
}

void Match_Destroy(MATCH_HANDLE head)
{
    MATCH_S *ctrl = head;

    BITMAP_Destory(&ctrl->bitmap);
    MEM_Free(ctrl);
}

int Match_SetPattern(MATCH_HANDLE head, int index, void *pattern)
{
    MATCH_S *ctrl = head;

    BS_DBGASSERT(index >= 0);
    BS_DBGASSERT(index < ctrl->max);

    memcpy(ctrl->nodes[index].pattern, pattern, ctrl->pattern_size);

    return 0;
}

void * Match_GetPattern(MATCH_HANDLE head, int index)
{
    MATCH_S *ctrl = head;

    BS_DBGASSERT(index >= 0);
    BS_DBGASSERT(index < ctrl->max);

    return ctrl->nodes[index].pattern;
}

void Match_Enable(MATCH_HANDLE head, int index)
{
    MATCH_S *ctrl = head;

    BS_DBGASSERT(index >= 0);
    BS_DBGASSERT(index < ctrl->max);

    BITMAP_SET(&ctrl->bitmap, index);
}

void Match_Disable(MATCH_HANDLE head, int index)
{
    MATCH_S *ctrl = head;

    BS_DBGASSERT(index >= 0);
    BS_DBGASSERT(index < ctrl->max);

    BITMAP_CLR(&ctrl->bitmap, index);
}

BOOL_T Match_IsEnable(MATCH_HANDLE head, int index)
{
    MATCH_S *ctrl = head;

    BS_DBGASSERT(index >= 0);
    BS_DBGASSERT(index < ctrl->max);

    if (BITMAP_ISSET(&ctrl->bitmap, index)) {
        return TRUE;
    }

    return FALSE;
}

void Match_SetUD(MATCH_HANDLE head, int index, void *ud)
{
    MATCH_S *ctrl = head;

    BS_DBGASSERT(index >= 0);
    BS_DBGASSERT(index < ctrl->max);

    ctrl->nodes[index].ud = ud;
}

void * Match_GetUD(MATCH_HANDLE head, int index)
{
    MATCH_S *ctrl = head;

    BS_DBGASSERT(index >= 0);
    BS_DBGASSERT(index < ctrl->max);

    return ctrl->nodes[index].ud;
}

static int match_GetByKey(MATCH_S *ctrl, void *key)
{
    int index;

    BITMAP_SCAN_SETTED_BEGIN(&ctrl->bitmap, index) {
        if (TRUE == ctrl->pfcmp(ctrl->nodes[index].pattern, key)) {
            return index;
        }
    }BITMAP_SCAN_END();

    return -1;
}

/* 返回Match的index */
int Match_Do(MATCH_HANDLE head, void *key)
{
    MATCH_S *ctrl = head;
    int index;

    index = match_GetByKey(ctrl, key);
    if (index < 0) {
        return -1;
    }

    ctrl->nodes[index].match_count ++;

    return index;
}

UINT64 Match_GetMatchedCount(MATCH_HANDLE head, int index)
{
    MATCH_S *ctrl = head;

    BS_DBGASSERT(index >= 0);
    BS_DBGASSERT(index < ctrl->max);

    return ctrl->nodes[index].match_count;
}

void Match_ResetMatchedCount(MATCH_HANDLE head, int index)
{
    MATCH_S *ctrl = head;

    BS_DBGASSERT(index >= 0);
    BS_DBGASSERT(index < ctrl->max);

    ctrl->nodes[index].match_count = 0;
}

void Match_ResetAllMatchedCount(MATCH_HANDLE head)
{
    MATCH_S *ctrl = head;
    int i;

    for (i=0; i<ctrl->max; i++) {
        Match_ResetMatchedCount(head, i);
    }
}

