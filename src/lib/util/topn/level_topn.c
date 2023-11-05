/*********************************************************
*   Copyright (C) LiXingang
*   Description: 对数据进行分级
*
********************************************************/
#include "bs.h"
#include "utl/level_topn.h"


static inline U64 _level_topn_get_level_score(LEVEL_TOPN_S *topn, int level)
{
    return topn->score_step * level;
}


static inline int _level_topn_get_score_level(LEVEL_TOPN_S *topn, U64 score)
{
    unsigned int level = score / topn->score_step;
    return MIN(level, (topn->level_count - 1));
}


static inline void _level_topn_cacl_low_level(LEVEL_TOPN_S *topn)
{
    if (unlikely(topn->level_admissions[topn->cur_low_level] < 0)) {
        while (topn->level_admissions[topn->cur_low_level] < 0) {
            topn->cur_low_level ++;
        }
        topn->low_level_score = _level_topn_get_level_score(topn, topn->cur_low_level);
    }
}


static inline void _level_topn_not_full_process(LEVEL_TOPN_S *topn, int level, int value)
{
    int index = topn->admited_count;
    LEVEL_TOPN_NODE_S *node = &topn->admissions[index];

    node->value = value;
    node->next = topn->level_admissions[level];
    topn->level_admissions[level] = index;

    topn->admited_count ++;

    if (likely(topn->admited_count < topn->n)) {
        
        return;
    }

    
    _level_topn_cacl_low_level(topn);
}


static inline void _level_topn_full_process(LEVEL_TOPN_S *topn, int level, int value)
{
    
    int index = topn->level_admissions[topn->cur_low_level];
    topn->level_admissions[topn->cur_low_level] = topn->admissions[index].next;

    
    topn->admissions[index].next = topn->level_admissions[level];
    topn->level_admissions[level] = index;
    topn->admissions[index].value = value;

    _level_topn_cacl_low_level(topn);
}

static inline void _level_topn_level_input(LEVEL_TOPN_S *topn, int level, int value)
{
    
    if (topn->admited_count < topn->n) {
        _level_topn_not_full_process(topn, level, value);
        return;
    }

    
    if (topn->cur_low_level < level) {
        _level_topn_full_process(topn, level, value);
        return;
    }

    return;
}

void LevelTopn_Init(OUT LEVEL_TOPN_S *topn, int n, int level_count, U64 score_step,
        int *level_admissions, LEVEL_TOPN_NODE_S *admissions)
{
    topn->n = n;
    topn->level_count = level_count;
    topn->score_step = score_step;
    topn->level_admissions = level_admissions;
    topn->admissions = admissions;

    LevelTopn_Reset(topn);
}

LEVEL_TOPN_S * LevelTopn_Create(int n, int level_count, U64 score_step )
{
    LEVEL_TOPN_S *topn;
    int mem_size;
    int *level_admission;
    LEVEL_TOPN_NODE_S *nodes;

    mem_size = sizeof(LEVEL_TOPN_S) 
        + (sizeof(int) * level_count)
        + (sizeof(LEVEL_TOPN_NODE_S) * n);

    topn = MEM_Malloc(mem_size);
    if (! topn) {
        return NULL;
    }

    level_admission = (void*)(topn + 1);
    nodes = (void*)(level_admission + level_count);

    LevelTopn_Init(topn, n, level_count, score_step, level_admission, nodes);

    return topn;
}

void LevelTopn_Destroy(LEVEL_TOPN_S *topn)
{
    if (topn) {
        MEM_Free(topn);
    }
}


void LevelTopn_Reset(LEVEL_TOPN_S *topn)
{
    if (! topn) {
        return;
    }

    topn->admited_count = 0;
    topn->cur_low_level = 0;
    topn->low_level_score = 0;

    
    memset(topn->level_admissions, 0xff, topn->level_count * sizeof(int));
}


void LevelTopn_ScoreInput(LEVEL_TOPN_S *topn, U64 score, int value)
{
    if (score < topn->low_level_score) {
        return; 
    }

    int level = _level_topn_get_score_level(topn, score);

    _level_topn_level_input(topn, level, value);
}


void LevelTopn_LevelInput(LEVEL_TOPN_S *topn, int level, int value)
{
    _level_topn_level_input(topn, level, value);
}

void LevelTopn_PrintLevel(LEVEL_TOPN_S *topn, int level)
{
    int index;

    printf("[level %d] \n", level);

    index = topn->level_admissions[level];

    while (index >= 0) {
        printf(" %d ", topn->admissions[index].value);
        index = topn->admissions[index].next;
    }

    printf("\n");
}

void LevelTopn_Print(LEVEL_TOPN_S *topn)
{
    int i;

    for (i=topn->cur_low_level; i<topn->level_count; i++) {
        if (topn->level_admissions[i] < 0) {
            continue;
        }
        LevelTopn_PrintLevel(topn, i);
    }
}

