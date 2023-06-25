/*********************************************************
*   Copyright (C) LiXingang
*   Description: 对数据进行分级
*
********************************************************/
#include "bs.h"
#include "utl/level_topn.h"

/* 根据分数获取其对应的级别 */
static inline int _level_topn_get_level_by_score(LEVEL_TOPN_S *topn, U64 score)
{
    int i;

    for (i=topn->level_count - 1; i>=topn->low_level; i--) {
        if (score >= topn->level_scores[i]) {
            return i;
        }
    }

    return -1;
}

/* 计算low level */
static inline void _level_topn_cacl_low_level(LEVEL_TOPN_S *topn)
{
    while (topn->level_admissions[topn->low_level] < 0) {
        topn->low_level ++;
    }
}

/* 还未录取满,直接录取 */
static inline void _level_topn_not_full_process(LEVEL_TOPN_S *topn, int level, int value)
{
    int index = topn->admit_count;
    LEVEL_TOPN_NODE_S *node = &topn->admissions[index];

    node->value = value;
    node->next = topn->level_admissions[level];
    topn->level_admissions[level] = index;

    topn->admit_count ++;

    if (topn->admit_count < topn->n) {
        /* 还没录取满,直接返回 */
        return;
    }

    /* 录取满了, 记录最低的分数级别, 准备开始淘汰 */
    _level_topn_cacl_low_level(topn);
}

/* 已经录取满,开始进行淘汰 */
static inline void _level_topn_full_process(LEVEL_TOPN_S *topn, int level, int value)
{
    /* 从low level中拿一个出来 */
    int index = topn->level_admissions[topn->low_level];
    topn->level_admissions[topn->low_level] = topn->admissions[index].next;

    /* 将这个新的通知书加入level中 */
    topn->admissions[index].next = topn->level_admissions[level];
    topn->level_admissions[level] = index;
    topn->admissions[index].value = value;

    _level_topn_cacl_low_level(topn);
}

void LevelTopn_Init(OUT LEVEL_TOPN_S *topn, int n, int level_count,
        U64 *level_scores, int *level_admissions,LEVEL_TOPN_NODE_S *admissions)
{
    topn->n = n;
    topn->level_count = level_count;
    topn->level_scores = level_scores;
    topn->level_admissions = level_admissions;
    topn->admissions = admissions;

    LevelTopn_Reset(topn);
}

LEVEL_TOPN_S * LevelTopn_Create(int n, int level_count)
{
    LEVEL_TOPN_S *topn;
    int mem_size;
    U64 *level_scores;
    int *level_admission;
    LEVEL_TOPN_NODE_S *nodes;

    mem_size = sizeof(LEVEL_TOPN_S) 
        + (sizeof(U64) * level_count)
        + (sizeof(int) * level_count)
        + (sizeof(LEVEL_TOPN_NODE_S) * n);

    topn = MEM_Malloc(mem_size);
    if (! topn) {
        return NULL;
    }

    level_scores = (void*)(topn + 1);
    level_admission = (void*)(level_scores + level_count);
    nodes = (void*)(level_admission + level_count);

    LevelTopn_Init(topn, n, level_count, level_scores, level_admission, nodes);

    return topn;
}

void LevelTopn_Destroy(LEVEL_TOPN_S *topn)
{
    if (topn) {
        MEM_Free(topn);
    }
}

/* 设置对应级别的分数线 */
void LevelTopn_SetLevel(LEVEL_TOPN_S *topn, int level, U64 score)
{
    if (! topn) {
        return;
    }

    if (level >= topn->level_count) {
        return;
    }

    topn->level_scores[level] = score;
}

/* 复位录取状态 */
void LevelTopn_Reset(LEVEL_TOPN_S *topn)
{
    int i;

    if (! topn) {
        return;
    }

    topn->admit_count = 0;
    topn->low_level = 0;

    for (i=0; i<topn->level_count; i++) {
        topn->level_admissions[i] = -1;
    }
}

/* 还没有录取满, 取一个空的录取通知书 */
void LevelTopn_Input(LEVEL_TOPN_S *topn, U64 score, int value)
{
    if (score < topn->level_scores[topn->low_level]) {
        return; /* 不予录取 */
    }

    int level = _level_topn_get_level_by_score(topn, score);

    /* 还没有录取满,直接录取 */
    if (topn->admit_count < topn->n) {
        _level_topn_not_full_process(topn, level, value);
        return;
    }

    /* 已经录取满了,看是否需要淘汰掉一个 */
    if (topn->low_level < level) {
        _level_topn_full_process(topn, level, value);
        return;
    }

    return;
}

void LevelTopn_PrintLevel(LEVEL_TOPN_S *topn, int level)
{
    int index;

    printf("level %d score %llu\n", level, topn->level_scores[level]);

    index = topn->level_admissions[level];

    while (index >= 0) {
        printf("%d ", topn->admissions[index].value);
        index = topn->admissions[index].next;
    }

    printf("\n");
}

void LevelTopn_Print(LEVEL_TOPN_S *topn)
{
    int i;

    for (i=0; i<topn->level_count; i++) {
        if (topn->level_admissions[i] < 0) {
            continue;
        }
        LevelTopn_PrintLevel(topn, i);
    }
}

