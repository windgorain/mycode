/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-2-28
* Description: 
* History:     
******************************************************************************/

#ifndef __TIME_UTL_H_
#define __TIME_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#include <time.h>

#define TM_STRING_TIME_LEN 47

#define DAYS_2_SECONDS(days) ((days) * 24 * 60 * 60)

/* 返回UTC时间 */
static inline time_t TM_NowInSec(void)
{
	return time(0);
}

/* 从系统启动到现在的秒数 */
ULONG TM_SecondsFromInit(void);
/* 从系统启动到现在的毫秒数 */
UINT64 TM_MsFromInit(void);
/* 从系统启动到现在的us数 */
UINT64 TM_UsFromInit(void);
/* 从系统启动到现在的ns数 */
UINT64 TM_NsFromInit(void);
/* 转换成如下格式:Fri, 29 Feb 2008 12:20:34  */
VOID TM_Utc2String(IN time_t ulUtcTime, OUT CHAR *szStringTime);
/* UTC时间转换成日期时分秒 */
VOID TM_Utc2Tm(IN time_t ulUtcTime, OUT struct tm *pstTm);
/* 日期时分秒转换成UTC */
BS_STATUS TM_Tm2Utc(IN struct tm *pstTm, OUT time_t *pulUtcTime);

/* 转换成如下格式:Fri, 29 Feb 2008 12:20:34  */
char * TM_Tm2String(struct tm *pstTm, OUT char *szStringTime);

BS_STATUS TM_String2Utc(IN CHAR *pszStringTime, OUT time_t *pulUtcTime);
BS_STATUS TM_String2Tm(IN CHAR *pszStringTime, OUT struct tm *pstTm);

/* 转换成asctime格式字符串: Sat Mar 25 06:10:10 1989  */
char * TM_Utc2Acstime(time_t seconds, OUT char *string);

/* 转换成格式: 2014-11-28 16:43:38 */
char * TM_Utc2SimpleString(time_t ulUtcTime, OUT CHAR *szTimeString);
/* 转换成如下格式:Fri, 29 Feb 2008 12:20:34  */
char * TM_Tm2SimpleString(struct tm *pstTm, OUT CHAR *szStringTime);
/*******************************************************************************
  将ULONG转换成符合RFC1123规定的时间字符串
*******************************************************************************/
BS_STATUS TM_Utc2Gmt(IN time_t stTimeSrc, OUT CHAR *szDataStr);
time_t TM_Gmt2Utc(IN CHAR *pucValue, IN ULONG ulLen);

char * TM_GetTimeString(UINT input_time, OUT char *string, int size);

/* HZ */
extern unsigned long TM_HZ; /* 一秒的tick数 */
extern unsigned long TM_MS_HZ; /* 一ms的tick数 */

unsigned long TM_GetTickPerSec(void);

static inline unsigned long TM_GetTick(void)
{
    unsigned long now_tick;
    struct tms buf;
    now_tick = times(&buf);
    return now_tick * 10;
}

#if 1 /* 获取程序当前的CPU 运行 clock, sleep消耗的时间不算 */
static inline long TM_GetClock(void) {
        return clock();
}

static inline long TM_GetClockPerSec(void) {
        return CLOCKS_PER_SEC;
}
#endif


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__TIME_UTL_H_*/


