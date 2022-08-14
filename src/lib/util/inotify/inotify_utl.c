/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/inotify_utl.h"

#ifdef IN_LINUX
static int _os_inotify_init(int flag)
{
    return inotify_init1(flag);
}

static int _os_inotify_add_watch(int fd, char *pathname, UINT mask)
{
    return inotify_add_watch(fd, pathname, mask);
}

static int _os_inotify_rm_watch(int fd, int wd)
{
    return inotify_rm_watch(fd, wd);
}
#else 
static int _os_inotify_init(int flag)
{
    RETURN(BS_NOT_SUPPORT);
}

static int _os_inotify_add_watch(int fd, char *pathname, UINT mask)
{
    RETURN(BS_NOT_SUPPORT);
}

static int _os_inotify_rm_watch(int fd, int wd)
{
    RETURN(BS_NOT_SUPPORT);
}
#endif

int INOTIFY_Init(int flag)
{
    return _os_inotify_init(flag);
}

int INOTIFY_AddWatch(int fd, IN char *pathname, UINT mask)
{
    return _os_inotify_add_watch(fd, pathname, mask);
}

int INOTIFY_DelWatch(int fd, int wd)
{
    return _os_inotify_rm_watch(fd, wd);
}




