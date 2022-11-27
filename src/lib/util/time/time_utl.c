/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-2-28
* Description: 性能: rdtsc > gettimeofday > clock_gettime > times
* History:     
******************************************************************************/

/* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_TIME

#include "bs.h"

#include "utl/time_utl.h"
#include "utl/txt_utl.h"

#define HTTP_TIME_REMAIN_MAX_LEN  18
#define HTTP_TIME_ISOC_REMAIN_LEN 14
#define HTTP_HEAD_FIELD_SPLIT_CHAR         ':'                     /* head field中的分隔符 */

/* 对于描述时间的字符串，不同的rfc标准，格式也不一样 */
typedef enum tagHTTP_TimeStandard
{
    NORULE = 0,
    RFC822,   /* Tue, 10 Nov 2002 23:50:13   */
    RFC850,   /* Tuesday, 10-Dec-02 23:50:13 */
    ISOC,     /* Tue Dec 10 23:50:13 2002    */
} HTTP_TIME_STANDARD_E;

/* var */
static const char *g_aucTimeWeekDay[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char *g_aucTimeMonth[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static UCHAR g_aucDaysOfMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
unsigned long TM_HZ = 0;   /* 一秒的tick数 */
unsigned long TM_MS_HZ = 0; /* 一U秒的tick数 */

#ifdef IN_WINDOWS
static inline UINT64 _tm_os_GetNsFromInit(void)
{
    return (UINT64)GetTickCount();
}
#endif

#ifdef IN_UNIXLIKE
static inline UINT64 _tm_os_GetNsFromInit(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return ((UINT64)ts.tv_sec * 1000000000 + (UINT64)ts.tv_nsec);
}
#endif

/* 转换成如下格式:Fri, 29 Feb 2008 12:20:34  */
VOID TM_Utc2String(IN time_t ulUtcTime, OUT CHAR *szStringTime)
{
    struct tm stTm;

    TM_Utc2Tm(ulUtcTime, &stTm);
    TM_Tm2String(&stTm, szStringTime);
}

BS_STATUS TM_String2Utc(IN CHAR *pszStringTime, OUT time_t *pulUtcTime)
{
    struct tm stTm;
    BS_STATUS eRet;

    if (BS_OK != (eRet = TM_String2Tm(pszStringTime, &stTm)))
    {
        return eRet;
    }

    if (BS_OK != (eRet = TM_Tm2Utc(&stTm, pulUtcTime)))
    {
        return eRet;
    }

    return BS_OK;
}

/* UTC时间转换成日期时分秒 */
VOID TM_Utc2Tm(IN time_t ulUtcTime, OUT struct tm *pstTm)
{
#ifdef IN_WINDOWS
    {
         localtime_s(pstTm, &ulUtcTime);
    }
#else
    {
        localtime_r(&ulUtcTime, pstTm);
    }
#endif
}

/* 日期时分秒转换成UTC */
BS_STATUS TM_Tm2Utc(IN struct tm *pstTm, OUT time_t *pulUtcTime)
{
    time_t lTime;

    lTime = mktime(pstTm);
    if (-1 == lTime)
    {
        RETURN(BS_ERR);
    }

    *pulUtcTime = lTime;

    return BS_OK;
}

/* 从String格式(如Fri, 29 Feb 2008 12:20:34)  转换成Tm格式*/
BS_STATUS TM_String2Tm(IN CHAR *pszStringTime, OUT struct tm *pstTm)
{
    UINT i;
    CHAR szTmp[8];
    BS_STATUS eRet;

    if (strlen(pszStringTime) < TM_STRING_TIME_LEN)
    {
        RETURN(BS_BAD_PARA);
    }
    
    /* 转换weekday */
    for (i=0; i<sizeof(g_aucTimeWeekDay)/sizeof(CHAR *); i++)
    {
        if (memcmp(g_aucTimeWeekDay[i], pszStringTime, 3) == 0)
        {
            pstTm->tm_wday = i;
            break;
        }
    }

    if (i >= sizeof(g_aucTimeWeekDay)/sizeof(CHAR *))
    {
        RETURN(BS_NOT_SUPPORT);
    }

    /* 转换month */
    for (i=0; i<sizeof(g_aucTimeMonth)/sizeof(CHAR*); i++)
    {
        if (memcmp(g_aucTimeMonth[i], pszStringTime + 8, 3) == 0)
        {
            pstTm->tm_mon = i;
            break;
        }
    }

    if (i >= sizeof(g_aucTimeMonth)/sizeof(CHAR *))
    {
        RETURN(BS_NOT_SUPPORT);
    }

    /* 转换日 */
    memcpy(szTmp, pszStringTime + 5, 2);
    szTmp[2] = '\0';

    eRet = TXT_Atoui(szTmp, &i);
    if (BS_OK != eRet)
    {
        return eRet;
    }

    if ((i < 1) || (i > 31))
    {
        RETURN(BS_NOT_SUPPORT);
    }

    pstTm->tm_mday = i;

    /* 转换年 */
    memcpy(szTmp, pszStringTime + 12, 4);
    szTmp[4] = '\0';

    eRet = TXT_Atoui(szTmp, &i);
    if (BS_OK != eRet)
    {
        return eRet;
    }

    if (i < 1900)
    {
        RETURN(BS_NOT_SUPPORT);
    }

    pstTm->tm_year = i - 1900;

    /* 计算year day */
    pstTm->tm_yday = 0;
    for (i=0; i<(UINT)pstTm->tm_mon; i++)
    {
        pstTm->tm_yday += g_aucDaysOfMonth[i];
    }

    if (pstTm->tm_mon > 2)
    {
        /* 瑞年2月要加一天 */
        pstTm->tm_yday += (pstTm->tm_year % 4 == 0) ? 1 : 0;
    }

    pstTm->tm_yday += pstTm->tm_mday;


    /* 转换时 */
    memcpy(szTmp, pszStringTime + 17, 2);
    szTmp[2] = '\0';

    eRet = TXT_Atoui(szTmp, &i);
    if (BS_OK != eRet)
    {
        return eRet;
    }

    if (i > 23)
    {
        RETURN(BS_NOT_SUPPORT);
    }

    pstTm->tm_hour = i;    

    
    /* 转换分 */
    memcpy(szTmp, pszStringTime + 20, 2);
    szTmp[2] = '\0';

    eRet = TXT_Atoui(szTmp, &i);
    if (BS_OK != eRet)
    {
        return eRet;
    }

    if (i > 59)
    {
        RETURN(BS_NOT_SUPPORT);
    }

    pstTm->tm_min = i;    

    
    /* 转换秒 */
    memcpy(szTmp, pszStringTime + 23, 2);
    szTmp[2] = '\0';

    eRet = TXT_Atoui(szTmp, &i);
    if (BS_OK != eRet)
    {
        return eRet;
    }

    if (i > 59)
    {
        RETURN(BS_NOT_SUPPORT);
    }

    pstTm->tm_sec = i;

    pstTm->tm_isdst = 0;

    return BS_OK;
}

/* 转换成如下格式:Fri, 29 Feb 2008 12:20:34  */
char * TM_Tm2String(IN struct tm *pstTm, OUT CHAR *szStringTime)
{
    sprintf(szStringTime, "%s, %02d %s %04d %02d:%02d:%02d",
        g_aucTimeWeekDay[pstTm->tm_wday], pstTm->tm_mday, 
        g_aucTimeMonth[pstTm->tm_mon], pstTm->tm_year + 1900,
        pstTm->tm_hour, pstTm->tm_min, pstTm->tm_sec);

    return szStringTime;
}

/* 转换成asctime格式字符串: Sat Mar 25 06:10:10 1989  */
char * TM_Utc2Acstime(time_t seconds, OUT char *string)
{
    struct tm stTm;
    TM_Utc2Tm(seconds, &stTm);
    asctime_r(&stTm, string);
    string[strlen(string)-1] = '\0'; /* 去掉最后的换行符 */
    return string;
}

/* 转换成格式: 2014-11-28 16:43:38, 本地时区时间 */
char * TM_Utc2SimpleString(time_t ulUtcTime, OUT CHAR *szTimeString)
{
    struct tm stTm;

    TM_Utc2Tm(ulUtcTime, &stTm);
    return TM_Tm2SimpleString(&stTm, szTimeString);
}

/* 转换成格式: 2014-11-28 16:43:38 */
char * TM_Tm2SimpleString(struct tm *pstTm, OUT CHAR *szStringTime)
{
    sprintf(szStringTime, "%04d-%02d-%02d %02d:%02d:%02d",
         pstTm->tm_year + 1900, pstTm->tm_mon + 1, pstTm->tm_mday, 
        pstTm->tm_hour, pstTm->tm_min, pstTm->tm_sec);

    return szStringTime;
}

/*******************************************************************************
  将ULONG转换成符合RFC1123规定的时间字符串
*******************************************************************************/
BS_STATUS TM_Utc2Gmt(IN time_t stTimeSrc, OUT CHAR *szDataStr)
{    
    LONG   lYday;
    ULONG  ulTime, ulSec, ulMin, ulHour, ulMday, ulMon, ulYear, ulWday, ulDays, ulLeap;
    ULONG  ulTemp;
    

    /* the calculation is valid for positive time_t only */

    ulTime =  stTimeSrc;

    ulDays = ulTime / 86400;

    /* Jaunary 1, 1970 was Thursday */

    ulWday = (4 + ulDays) % 7;

    ulTime %= 86400;
    ulHour = ulTime / 3600;
    ulTime %= 3600;
    ulMin = ulTime / 60;
    ulSec = ulTime % 60;

    /*
     * the algorithm based on Gauss' formula,
     * see src/http/ngx_http_parse_time.c
     */

    /* days since March 1, 1 BC */
    ulDays = (ulDays - (31 + 28)) + 719527;

    /*
     * The "days" should be adjusted to 1 only, however, some March 1st's go
     * to previous year, so we adjust them to 2.  This causes also shift of the
     * last Feburary days to next year, but we catch the case when "yday"
     * becomes negative.
     */

    ulYear = ((ulDays + 2) * 400 )/ (((365 * 400) + (100 - 4)) + 1);

    ulTemp = (365 * ulYear) + (ulYear / 4);
    ulTemp = ulTemp - (ulYear / 100);
    ulTemp = ulTemp + (ulYear / 400);
    lYday = (LONG)ulDays - (LONG)ulTemp;

    if (lYday < 0) {
        if((0 == (ulYear % 4)) && (((ulYear % 100) != 0) || ((ulYear % 400) == 0)))
        {
            ulLeap = 1;
        }
        else
        {
            ulLeap = 0;
        }
        lYday = (365 + (LONG)ulLeap) + lYday;
        ulYear--;
    }

    /*
     * The empirical formula that maps "yday" to month.
     * There are at least 10 variants, some of them are:
     *     mon = (yday + 31) * 15 / 459
     *     mon = (yday + 31) * 17 / 520
     *     mon = (yday + 31) * 20 / 612
     */

    ulMon = (((ULONG)lYday + 31) * 10) / 306;

    /* the Gauss' formula that evaluates days before the month */

    ulMday = ((ULONG)lYday - (((367 * ulMon) / 12) - 30)) + 1;

    if (lYday >= 306) {

        ulYear++;
        ulMon -= 10;

        /*
         * there is no "yday" in Win32 SYSTEMTIME
         *
         * yday -= 306;
         */

    } else {

        ulMon += 2;

        /*
         * there is no "yday" in Win32 SYSTEMTIME
         *
         * yday += 31 + 28 + leap;
         */
    }    

    if( ( ulWday >= 7 ) || ( ulMon > 12 ) || ( ulMon < 1 ) )
    {
        return BS_ERR;
    }

    /* 将时间转换成协议要求的字符串格式 */
    (VOID)snprintf( szDataStr, TM_STRING_TIME_LEN + 1, "%3s, %02lu %3s %lu %02lu:%02lu:%02lu GMT",
                    g_aucTimeWeekDay[ulWday],
                    ulMday,
                    g_aucTimeMonth[ulMon - 1],
                    ulYear,
                    ulHour,
                    ulMin,
                    ulSec );
    
    return BS_OK;
}

time_t TM_Gmt2Utc(IN CHAR *pcValue, IN ULONG ulLen)
{
    UCHAR *pucPos;
    UCHAR *pucEnd;
    INT iMonth;
    UINT uiDay;
    UINT uiYear;
    UINT uiHour;
    UINT uiMin;
    UINT uiSec;
    ULONG ulTime;
    HTTP_TIME_STANDARD_E enFmt;
    UCHAR *pucValue = (UCHAR*)pcValue;

    /* 入参合法性检查 */
    if((NULL == pucValue)||(0 == ulLen))
    {
        return 0;
    }

    enFmt = NORULE;
    uiDay = 0;
    uiYear = 0;
    uiHour = 0;
    uiMin = 0;
    uiSec = 0;
    pucPos = pucValue;
    pucEnd = pucValue + ulLen;

    /* 看先找到','还是' ',如果先找到空格则是按ISOC处理 */
    for (pucPos = pucValue; pucPos < pucEnd; pucPos++) 
    {
        if (',' == *pucPos) 
        {
            break;
        }

        if (' '==*pucPos) 
        {
            enFmt = ISOC;
            break;
        }
    }

    /* 
        找到空格
        对Tue, 10 Nov 2002 23:50:13来说，是找到10开始的地方
        对Tue Dec 10 23:50:13 2002来说，是找到10开始的地方
    */
    for (pucPos++; pucPos < pucEnd; pucPos++)
    {
        if (' ' != *pucPos) 
        {
            break;
        }
    }

    /*  上面提到的三种rfc规则，最短的是ISOC，找到两个空格后面的数据长度至少是18 */
    if (HTTP_TIME_REMAIN_MAX_LEN > (pucEnd - pucPos)) 
    {
        return 0;
    }

    if (ISOC != enFmt) 
    {
        if (('0' > *pucPos) || 
            ('9' < *pucPos) || 
            ('0' > *(pucPos + 1)) || 
            ('9' < *(pucPos + 1))) 
        {
            return 0;
        }
        /* 得到天 Tue, 10 Nov 2002 23:50:13中的10 */
        uiDay = (((*pucPos - '0') * 10) + (*(pucPos + 1))) - '0';
        /*  
            此时p要么指向Tue, 10 Nov 2002 23:50:13中的 Nov...
            要么指向Tuesday, 10-Dec-02 23:50:13中的-Dec-02 23:50:13
        */
        pucPos += 2; 
        if (' ' == *pucPos) 
        {
            if (HTTP_TIME_REMAIN_MAX_LEN > (pucEnd - pucPos)) 
            {
                return 0;
            }
            /* Tue, 10 Nov 2002 23:50:13情况 */
            enFmt = RFC822;

        } 
        else if ('-' == *pucPos) 
        {
            /* Tuesday, 10-Dec-02 23:50:13情况 */
            enFmt = RFC850;
        } 
        else 
        {
            return 0;
        }

        pucPos++;
    }

    /* 开始处理月份 :Nov 2002 23:50:13或者Dec-02 23:50:13或者Dec 10 23:50:13 2002*/
    switch (*pucPos) 
    {

        case 'J':
        {
            iMonth = (*(pucPos + 1) == 'a' )? 0 : ((*(pucPos + 2) == 'n' )? 5 : 6);   /* JAN JUNE JULY */
            break;
        }

        case 'F':
        {
            iMonth = 1;
            break;
        }

        case 'M':
        {
            iMonth = (*(pucPos + 2) == 'r') ? 2 : 4;  /* March, May */
            break;
        }

        case 'A':
        {
            iMonth = (*(pucPos + 1) == 'p') ? 3 : 7; /* Apirl, August */
            break;
        }
        case 'S':
        {
            iMonth = 8;
            break;
        }
        case 'O':
        {
            iMonth = 9;
            break;
        }
        case 'N':
        {
            iMonth = 10;
            break;
        }
        case 'D':
        {
            iMonth = 11;
            break;
        }
        default:
        {
            return 0;
        }
    }

    /* 前进三位，月份占三个字节 */
    pucPos += 3;

    /* 此时RFC822: 2002 23:50:13
           RFC850: -02 23:50:13
           ISOC: 10 23:50:13 2002 */
    if (((RFC822 == enFmt) && (' ' != *pucPos)) || 
        ((RFC850 == enFmt) && ('-' != *pucPos))) 
    {
        return 0;
    }

    pucPos++;
    /* 此时RFC822:2002 23:50:13
           RFC850:02 23:50:13
           ISOC:10 23:50:13 2002 */
    if (RFC822 == enFmt) 
    {
        if (('0' > *pucPos) || 
            ('9' < *pucPos) || 
            ('0' > *(pucPos + 1)) || 
            ('9' < *(pucPos + 1)) || 
            ('0' > *(pucPos + 2)) || 
            ('9' < *(pucPos + 2)) || 
            ('0' > *(pucPos + 3)) || 
            ('9' < *(pucPos + 3)))
        {
            return 0;
        }

        uiYear = ((*pucPos - '0') * 1000) + 
                 ((*(pucPos + 1) - '0') * 100 )+ 
                 ((*(pucPos + 2) - '0') * 10) + 
                  (*(pucPos + 3) - '0'); /* 得到2002 */
        pucPos += 4;

    } 
    else if (RFC850 == enFmt) 
    {
        if (('0' > *pucPos) || 
            ('9' < *pucPos) || 
            ('0' > *(pucPos + 1)) || 
            ('9' < *(pucPos + 1))) 
        {
            return 0;
        }

        uiYear = (((*pucPos - '0') * 10) + *(pucPos + 1)) - '0';
        uiYear += (uiYear < 70) ? 2000 : 1900; /* why 70? */
        pucPos += 2;
    }
    else
    {
        /* do nothing */
    }

    if (ISOC == enFmt) 
    {
        /* 当前p指向10 23:50:13 2002 */
        if (' ' == *pucPos) 
        {
            pucPos++;
        }

        if (('0' > *pucPos) || 
            ('9' < *pucPos)) 
        {
            return 0;
        }

        uiDay = *pucPos++ - '0'; /* 得到1 */

        if (' ' != *pucPos) 
        {
            if (('0' > *pucPos) || 
                ('9' < *pucPos)) 
            {
                return 0;
            }

            uiDay = ((uiDay * 10) + *pucPos++) - '0'; /* 得到10 */
        }
        /* 还剩 23:50:13 2002，长度为14 */
        if (HTTP_TIME_ISOC_REMAIN_LEN > (pucEnd - pucPos)) 
        {
            return 0;
        }
    }

    if (' ' != *pucPos++) 
    {
        return 0;
    }

    /* 
        此时RFC822:  23:50:13 
            RFC850:  23:50:13
            ISOC:    23:50:13 2002
            
    */
    if (('0' > *pucPos) || 
        ('9' < *pucPos) || 
        ('0' > *(pucPos + 1)) || 
        ('9' < *(pucPos + 1))) 
    {
        return 0;
    }

    /* 得到小时数 23 */
    uiHour = (((*pucPos - '0') * 10) + *(pucPos + 1)) - '0';
    pucPos += 2;

    if (HTTP_HEAD_FIELD_SPLIT_CHAR != *pucPos++) 
    {
        return 0;
    }

    if (('0' > *pucPos) || 
        ('9' < *pucPos) || 
        ('0' > *(pucPos + 1)) || 
        ('9' < *(pucPos + 1))) 
    {
        return 0;
    }
    /* 得到分钟数 50 */
    uiMin = (((*pucPos - '0') * 10) + *(pucPos + 1)) - '0';
    pucPos += 2;

    if (HTTP_HEAD_FIELD_SPLIT_CHAR != *pucPos++) 
    {
        return 0;
    }

    if (('0' > *pucPos) || 
        ('9' < *pucPos) || 
        ('0' > *(pucPos + 1)) || 
        ('9' < *(pucPos + 1))) 
    {
        return 0;
    }
    /* 得到秒数 13 */
    uiSec = (((*pucPos - '0') * 10) + *(pucPos + 1)) - '0';

    /* 如果是ISOC, 最后还有四个字节表示年份 */
    if (ISOC == enFmt) 
    {
        pucPos += 2;

        if (' ' != *pucPos++) 
        {
            return 0;
        }

        if (('0' > *pucPos) || 
            ('9' < *pucPos) || 
            ('0' > *(pucPos + 1)) || 
            ('9' < *(pucPos + 1)) || 
            ('0' > *(pucPos + 2)) || 
            ('9' < *(pucPos + 2)) || 
            ('0' > *(pucPos + 3)) || 
            ('9' < *(pucPos + 3)))
        {
            return 0;
        }

        uiYear = (((*pucPos - '0') * 1000) + 
                 ((*(pucPos + 1) - '0') * 100 )+ 
                 ((*(pucPos + 2) - '0') * 10) + 
                  *(pucPos + 3)) - '0';
    }

    if ((23 < uiHour) || 
        (59 < uiMin) || 
        (59 < uiSec)) 
    {
         return 0;
    }

    /* 如果二月份有29天 且不是闰年*/
    if ((29 == uiDay) && 
        (1 == iMonth)) 
    {
        if ((0 < (uiYear & 3)) || 
            ((0 == (uiYear % 100)) && (0 != (uiYear % 400)))) 
        {
            return 0;
        }

    }
    /* g_auiHttpDay[1]为28，二月份只有28和29天两种情况 */
    else if (uiDay > g_aucDaysOfMonth[iMonth]) 
    {
        return 0;
    }
    else
    {
        /* do nothing */
    }

    /*
        这是一个计算日期的高斯公式算法，以下注释出自nginx源码
        shift new year to March 1 and start months from 1 (not 0),
        it is needed for Gauss' formula
        
    */

    if (0 >= --iMonth) 
    {
        /* 
            新的一年从三月开始
            否则一月和二月退回到上一年(OMG..)
        */
        iMonth += 12;
        uiYear -= 1;
    }

    /* 
        高斯公式，标准算法
        出自nginx:
        Gauss' formula for Grigorian days since March 1, 1 BC 
        以下为nginx开源代码，为了避免pclint报警，修改代码格式
    
    ulTime =(
             //days in years including leap years since March 1, 1 BC 

            365 * uiYear + uiYear / 4 - uiYear / 100 + uiYear / 400

             //days before the month 

            + 367 * iMonth / 12 - 30

             //days before the day 

            + uiDay - 1

            
             // 719527 days were between March 1, 1 BC and March 1, 1970,
             // 31 and 28 days were in January and February 1970
             

            - 719527 + 31 + 28) * 86400 + uiHour * 3600 + uiMin * 60 + uiSec;
    */

    ulTime = (ULONG)365 * uiYear;
    ulTime += uiYear / 4;
    ulTime -= uiYear / 100;
    ulTime += uiYear / 400;
    ulTime += ((367 * (ULONG)((UINT)iMonth)) / 12);
    ulTime = (ulTime + uiDay) - 719499;
    ulTime = ulTime * 86400;
    ulTime += ((uiHour * 3600UL) + (uiMin * 60UL) + uiSec);

    return (time_t) ulTime;
}

/*
 * input_time: 0表示取当前时间
 * string: NULL表示使用内置静态变量,多线程环境下有可能覆盖 
 */
char * TM_GetTimeString(UINT input_time, OUT char *string, int size)
{
    struct tm *tm;
    static char tmp[64];

    if (! string) {
        string = tmp;
        size = sizeof(tmp);
    }

    if(input_time == 0){
        struct timeval tv;
        gettimeofday(&tv, NULL);
        tm = localtime(&tv.tv_sec);
    } else {
        time_t now=(time_t)input_time;
        tm = localtime(&now);
    }

    strftime(string, size, "%Y-%m-%d %H:%M:%S", tm);
/*
    snprintf(string, size, "%04d-%02d-%02d %02d:%02d:%02d.%06u",
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour,
            tm->tm_min, tm->tm_sec, (UINT) (tv.tv_usec));
*/

    return string;
}

/* 从系统启动到现在的ns数 */
UINT64 TM_NsFromInit(void)
{
    return _tm_os_GetNsFromInit();
}

/* 从系统启动到现在的us数 */
UINT64 TM_UsFromInit(void)
{
    return _tm_os_GetNsFromInit() / 1000;
}

/* 从系统启动到现在的ms数 */
UINT64 TM_MsFromInit(void)
{
    return _tm_os_GetNsFromInit() / 1000000;
}

/* 从系统启动到现在的秒数 */
ULONG TM_SecondsFromInit(void)
{
    return TM_UsFromInit()/1000000;
}

unsigned long TM_GetTickPerSec(void)
{
    unsigned long clocks;

    clocks = sysconf(_SC_CLK_TCK);

    return clocks * 10;
}

static void tm_HZInit(void)
{
    TM_HZ = TM_GetTickPerSec();
    TM_MS_HZ = TM_HZ/1000;
}

CONSTRUCTOR(init) {
    tm_HZInit();
}
