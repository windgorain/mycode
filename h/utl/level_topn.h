/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0
* Description:
******************************************************************************/
#ifndef _LEVEL_TOPN_H
#define _LEVEL_TOPN_H

typedef struct {
    int next;
    int value; /* 被录取者信息,如学号 */
    // U64 score;
}LEVEL_TOPN_NODE_S;

typedef struct {
    int n; /* 最多录取数目 */
    int admit_count; /* 已经录取数目 */
    int level_count; /* 总共多少级 */
    int low_level; /* 当前被录取者的最低级别 */
    U64 *level_scores; /* 分级分数线 */
    int *level_admissions; /* 每个级别的录取书 */
    LEVEL_TOPN_NODE_S *admissions; /* 录取通知书 */
}LEVEL_TOPN_S;


void LevelTopn_Init(OUT LEVEL_TOPN_S *topn, int n, int level_count,
        U64 *level_scores, int *level_admissions,LEVEL_TOPN_NODE_S *admissions);
LEVEL_TOPN_S * LevelTopn_Create(int n, int level_count);
void LevelTopn_Destroy(LEVEL_TOPN_S *topn);
void LevelTopn_SetLevel(LEVEL_TOPN_S *topn, int level, U64 score);
void LevelTopn_Reset(LEVEL_TOPN_S *topn);
void LevelTopn_Input(LEVEL_TOPN_S *topn, U64 score, int value);
void LevelTopn_PrintLevel(LEVEL_TOPN_S *topn, int level);
void LevelTopn_Print(LEVEL_TOPN_S *topn);

#endif //LEVEL_TOPN_H_
