/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _ARGS_UTL_H
#define _ARGS_UTL_H
#ifdef __cplusplus
extern "C"
{
#endif

/* 按照命令行分割argc/argv的方式,将数据分隔成多段, 返回分割成了多少个段 */
int ARGS_Split
(
    char *string,
    char **args,   /* 指向数组 */
    int max_count/* 数组中元素个数 */
);

#ifdef __cplusplus
}
#endif
#endif //ARGS_UTL_H_
