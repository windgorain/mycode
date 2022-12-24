/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-10-26
* Description: 
* History:     
******************************************************************************/

#ifndef __GETOPT2_UTL_H_
#define __GETOPT2_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define GETOPT2_OUT_FLAG_ISSET 0x1 /* 此选项被设置 */

typedef struct {
    UINT min;
    UINT max;
}GETOPT2_NUM_RANGE_S;

typedef struct
{
    char opt_type;    /* 'p': 不是opt而是参数,不以'-'开头; 'o': opt; 0: 无效 , P/O必选, p/o可选 */
    char opt_short_name; /* opt情况下是short opt name, param情况下无效 */
    char *opt_long_name; /* opt情况下是long opt name, param情况下,是param信息 */
/*
   'u': unsigned int 
   's': string, char *
   'b': bool, BOOL_T
   'i': ip, IP_PREFIX_S
   'r': ports, GETOPT2_NUM_RANGE_S, 格式: 80 或者 100-1000
 */
    char value_type; 
    void *value;
    char *help_info;
    unsigned int flag;
}GETOPT2_NODE_S;

int GETOPT2_ParseFromArgv0(UINT uiArgc, CHAR **ppcArgv, INOUT GETOPT2_NODE_S *opts);
int GETOPT2_Parse (UINT uiArgc, CHAR **ppcArgv, GETOPT2_NODE_S *pstNodes);
char * GETOPT2_BuildHelpinfo(GETOPT2_NODE_S *nodes, OUT char *buf, int buf_size);
/* short name 和 long name 只要输入一个有效的即可 */
int GETOPT2_IsOptSetted(GETOPT2_NODE_S *nodes, char short_opt_name, char *long_opt_name);
int GETOPT2_IsHaveError(GETOPT2_NODE_S *opts);
void GETOPT2_PrintHelp(GETOPT2_NODE_S *opts);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__GETOPT2_UTL_H_*/


