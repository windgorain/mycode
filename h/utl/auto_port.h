/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _AUTO_PORT_H
#define _AUTO_PORT_H
#ifdef __cplusplus
extern "C"
{
#endif

enum {
    AUTOPORT_TYPE_SET = 0, /* 指定端口号, 端口号为port */
    AUTOPORT_TYPE_INC,     /* 递增端口号,  端口范围为[v1,v2], 结果存到port */
    AUTOPORT_TYPE_PID,     /* PID端口号, 端口为v1 + pid & 0xfff, 结果存到port */
    AUTOPORT_TYPE_ADD,     /* 端口号为v1+v2, 结果存到port */
    AUTOPORT_TYPE_ANY,     /* 任意端口号, 由操作系统自动选择, 结果存到port */
};

typedef struct {
    USHORT port_type;
    USHORT port;
    USHORT v1;
    USHORT v2;
}AUTOPORT_S;

/* 失败返回<0, 成功返回端口号 */
typedef int (*PF_AUTOPORT_OPEN)(USHORT port, void *ud);

/* 失败返回<0, 成功返回port */
int AutoPort_Open(AUTOPORT_S *ap, PF_AUTOPORT_OPEN open_fn, void *ud);

#ifdef __cplusplus
}
#endif
#endif //AUTO_PORT_H_
