/*================================================================
*   Created by LiXingang: 2018.11.10
*   Description: 
*
================================================================*/
#include <fcntl.h>
#include "bs.h"

#include "utl/fd_utl.h"

void FD_SetNoBlock(int fd)
{
    int opts;
    opts = fcntl(fd, F_GETFL);
    opts |= O_NONBLOCK;
    fcntl(fd, F_SETFL, opts);
}

