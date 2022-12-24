/*================================================================
*   Created by LiXingang
*   Description: 封装文件监视器
*
================================================================*/
#ifndef _INOTIFY_UTL_H
#define _INOTIFY_UTL_H
#include "bs.h"
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef IN_LINUX

#include <sys/inotify.h>

#else

#define IN_NONBLOCK
#define IN_CLOEXEC

#define IN_ACCESS (1) /* 文件被访问 */
#define IN_ATTRIB (1<<1) /* 元数据被改变，例如权限、时间戳、扩展属性、链接数、UID、GID等 */
#define IN_CLOSE_WRITE (1<<2)  /* 关闭打开写的文件 */
#define IN_CLOSE_NOWRITE (1<<3) /* 和IN_CLOSE_WRITE刚好相反，关闭不是打开写的文件 */
#define IN_CREATE (1<<4)  /* 用于目录，在监控的目录中创建目录或文件时发生 */
#define IN_DELETE (1<<5) /* 用于目录，在监控的目录中删除目录或文件时发生 */
#define IN_DELETE_SELF (1<<6) /* 监控的目录或文件本身被删除 */
#define IN_MODIFY (1<<7) /* 文件被修改，这种事件会用到inotify_event中的cookie */
#define IN_MOVE_SELF (1<<8) /* 监控的文件或目录本身被移动 */
#define IN_MOVED_FROM (1<<9) /* 从监控的目录中移出文件 */
#define IN_MOVED_TO (1<<10) /* 向监控的目录中移入文件 */
#define IN_OPEN (1<<11)  /* 文件被打开 */
#define IN_ALL_EVENTS (0xfff)

/* 下面的标志位只在add中使用 */
#define IN_DONT_FOLLOW (1<<12) /* 如果监控的文件时一个符号链接，只监控符号链接本身，而不是链接指向的文件 */
#define IN_MASK_ADD (1<<13) /* 对已监控的文件增加要监控的的事件（不是替换原先的掩码）*/
#define IN_ONESHOT (1<<14) /* 只监控指定的文件一次，事件发生后从监控列表移除 */
#define IN_ONLYDIR (1<<15) /* 如果监控的是一个目录，只监控目录本身 */

/* 下面的这些bit位可能在read（）读取到的事件中设置 */
#define IN_IGNORED (1<<16) /* 监控被显式移除（调用inotify_rm_watch()），或者自动移除（文件被删除或者文件系统被卸载） */
#define IN_ISDIR (1<<17) /* 引发事件的是一个目录 */
#define IN_Q_OVERFLOW (1<<18) /* 事件队列溢出（这种情况下inotify_event结构中的wd为-1）*/
#define IN_UMOUNT (1<<19) /* 包含监控对象的文件系统被卸载 */

struct inotify_event {
   int      wd;       /* Watch descriptor */
   uint32_t mask;     /* Mask of events */
   uint32_t cookie;   /* Unique cookie associating related
                         events (for rename(2)) */
   uint32_t len;      /* Size of name field */
   char     name[];   /* Optional null-terminated name */
};

#endif

#define INOTIFY_EVENT_SIZE sizeof(struct inotify_event)

int INOTIFY_Init(int flag);
int INOTIFY_AddWatch(int fd, IN char *pathname, UINT mask);
int INOTIFY_DelWatch(int fd, int wd);

#ifdef __cplusplus
}
#endif
#endif //INOTIFY_UTL_H_
