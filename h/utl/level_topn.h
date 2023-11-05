/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0
* Description:
******************************************************************************/
#ifndef _LEVEL_TOPN_H
#define _LEVEL_TOPN_H

typedef struct {
    int next;
    int value; 
    
}LEVEL_TOPN_NODE_S;

typedef struct {
    unsigned int n; 
    unsigned int admited_count; 
    unsigned int level_count; 
    unsigned int cur_low_level; 
    U64 low_level_score; 
    U64 score_step; 
    int *level_admissions; 
    LEVEL_TOPN_NODE_S *admissions; 
}LEVEL_TOPN_S;


void LevelTopn_Init(OUT LEVEL_TOPN_S *topn, int n, int level_count, U64 score_step,
        int *level_admissions, LEVEL_TOPN_NODE_S *admissions);
LEVEL_TOPN_S * LevelTopn_Create(int n, int level_count, U64 score_step );
void LevelTopn_Destroy(LEVEL_TOPN_S *topn);
void LevelTopn_Reset(LEVEL_TOPN_S *topn);
void LevelTopn_ScoreInput(LEVEL_TOPN_S *topn, U64 score, int value);
void LevelTopn_LevelInput(LEVEL_TOPN_S *topn, int level, int value);
void LevelTopn_PrintLevel(LEVEL_TOPN_S *topn, int level);
void LevelTopn_Print(LEVEL_TOPN_S *topn);

#endif 
