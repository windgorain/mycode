/*================================================================
*   Created by LiXingang
*   Description: 进程index,从0开始计算
*
================================================================*/
#include "bs.h"
#include "utl/txt_utl.h"

#include<sys/file.h>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

int ProcessIndex_Get(char *index_file)
{
    int file = open(index_file, O_CREAT|O_RDWR, 0666);
    if (file < 0) {
        return -1;
    }

    int rc = flock(file, LOCK_EX);
    if (rc < 0) {
        close(file);
        return -1;
    } 

    char buf[16] = "";
    int index = 0;

    int ret = read(file, buf, sizeof(buf) - 1);
    if (ret >= 0) {
        buf[ret] = '\0';
        index = TXT_Str2Ui(buf);
    }

    
    if (ftruncate(file,0) < 0) {
    }

    lseek(file,0,SEEK_SET);
    sprintf(buf, "%d", index + 1);
    int len = strlen(buf);
    if (write(file, buf, len) != len) {
        index = -1;
    }
    flock(file, LOCK_UN);
    close(file);

    return index;
}

