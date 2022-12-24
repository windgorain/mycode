/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _SUBCMD_UTL_H
#define _SUBCMD_UTL_H
#ifdef __cplusplus
extern "C"
{
#endif

typedef int (*PF_SUBCMD_FUNC)(int argc, char **argv);

typedef struct {
    char *subcmd;
    void *func; /* 可以使用PF_SUBCMD_FUNC, 也可以自定义不同的函数类型 */
    char *help;
}SUB_CMD_NODE_S;

int SUBCMD_Do(SUB_CMD_NODE_S *subcmd, int argc, char **argv);

#ifdef __cplusplus
}
#endif
#endif //SUBCMD_UTL_H_
