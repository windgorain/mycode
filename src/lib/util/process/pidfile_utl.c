/*================================================================
*   Created by LiXingang: 2018.11.10
*   Description: 
*
================================================================*/
#include "bs.h"

#ifdef IN_UNIXLIKE
#include<sys/file.h>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include "utl/txt_utl.h"
#include "utl/pidfile_utl.h"
#include "utl/process_utl.h"


int PIDFILE_Create(char *filename)
{
    int fd = 0;
    char buf[512];

    fd = open(filename, O_RDWR|O_CREAT, 0664);
    if (fd == -1) {
        printf("Cannot open file %s ",filename);
        RETURN(BS_CAN_NOT_OPEN);
    }

    if (ftruncate(fd, 0) == -1) {
        printf( "Cannot truncate file %s \n", filename);
        close(fd);
        return -1;
    }

    snprintf(buf, sizeof(buf), "%ld\n", (long) getpid());
    if (write(fd, buf, strlen(buf)) != (ssize_t)strlen(buf)) {
        close(fd);
        return -1;
    }

    close(fd);

    return 0;
}

int PIDFILE_Lock(char *filename)
{
    int lock_file = open(filename, O_CREAT|O_RDWR, 0666);
    int rc = flock(lock_file, LOCK_EX|LOCK_NB);

    if (rc) {
        if (EWOULDBLOCK == errno) {
            printf("The program is probabaly is already running \n");
        }
        return -1;
    } 

    return 0;
}

int PIDFile_ReadPID(char *filename)
{
    int fd;
    char pid[16];

    fd = open(filename, O_RDWR, 0);
    if (fd < 0) {
        return -1;
    }

    int n = read(fd, pid, sizeof(pid) - 1);
    if (n <= 0) {
        close(fd);
        return -1;
    }

    close(fd);

    pid[n] = '\0';

    return TXT_Str2Ui(pid);
}


BOOL_T PIDFile_IsProcessRunning(char *pidfile)
{
    int pid = PIDFile_ReadPID(pidfile);

    if (pid <= 0) {
        return FALSE;
    }

    return PROCESS_IsPidExist(pid);
}

#endif
