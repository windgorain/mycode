/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2011-7-19
* Description: 
* History:     
******************************************************************************/
#include   "bs.h"
#include   "utl/getopt2_utl.h"
#include   "utl/socket_utl.h"
#include   "utl/mem_utl.h"
#include   <stdio.h>
#include   <netinet/in.h>  
#include   <netdb.h>  
#include   <sys/types.h>  
#include   <sys/stat.h>
#include   <sys/socket.h>  
#include   <arpa/inet.h>
#include   <unistd.h>  
#include   <sys/ioctl.h>  
#include   <sys/select.h>  
#include   <errno.h>   
#include   <dirent.h>
#include   <dlfcn.h>
#include   <signal.h>
#include   <pthread.h>
#include   <utime.h>
#include   <ncurses.h>
#include   <sys/un.h>
    
#define OFFSET(type,item) (unsigned int)(unsigned long)(&(((type *) 0)->item))

#define _PIPECMDC_FILE_NAME "/tmp/file_pipecmd"

static int _pipecmdc_Connect(const char *name)
{
	int					fd, len, err, rval;
	struct sockaddr_un	un;

	/* create a UNIX domain stream socket */
	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		return(-1);

	/* fill socket address structure with our address */
	memset(&un, 0, sizeof(un));
	un.sun_family = AF_UNIX;
	sprintf(un.sun_path, "/var/tmp/%05d", getpid());
	len = OFFSET(struct sockaddr_un, sun_path) + strlen(un.sun_path);

	unlink(un.sun_path);		/* in case it already exists */
	if (bind(fd, (struct sockaddr *)&un, len) < 0) {
		rval = -2;
		goto errout;
	}
	if (chmod(un.sun_path, S_IRWXU) < 0) {
		rval = -3;
		goto errout;
	}

	/* fill socket address structure with server's address */
	memset(&un, 0, sizeof(un));
	un.sun_family = AF_UNIX;
	strcpy(un.sun_path, name);
	len = OFFSET(struct sockaddr_un, sun_path) + strlen(name);
	if (connect(fd, (struct sockaddr *)&un, len) < 0) {
		rval = -4;
		goto errout;
	}

    int rcv_buf = 0x10000000; /* 10M */
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcv_buf, sizeof(int));

	return(fd);

errout:
	err = errno;
	close(fd);
	errno = err;
	return(rval);
}

static int _pipecmdc_Recv(int fd)
{
    char szBuf[1024];
    int iReadLen;
    int ret = 0;
    int over = 0;
    struct timeval tv_out;

    tv_out.tv_sec = 0;
    tv_out.tv_usec = 10000000; /* 10s */
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));

    while(1) {
        iReadLen = recv(fd, szBuf, sizeof(szBuf) - 1, 0);
        if (iReadLen < 0) {
            ret = -1;
            break;
        }

        if (iReadLen == 0) {
            break;
        }

        if (iReadLen > 0) {
            szBuf[iReadLen] = '\0';
            printf("%s", szBuf);
        }

        if (over) {
            ret = (UCHAR)szBuf[0];
            break;
        }

        if (szBuf[iReadLen - 1] == 0) {
            over = 1;
        }

        if (iReadLen >= 2) {
            if (szBuf[iReadLen - 2] == 0) {
                ret = (UCHAR)szBuf[iReadLen - 1];
                break;
            }
        }
    }

    return ret;
}

static void _pipecmdc_help(GETOPT2_NODE_S *opts)
{
    char buf[512];
    printf("%s", GETOPT2_BuildHelpinfo(opts, buf, sizeof(buf)));
    return;
}

static int _pipecmdc_SendFile(int fd, char *filename)
{
    FILE *fp;
    char *line;
    int len;
    int ret = 0;
    char buf[2048];

    fp = fopen(filename, "rb+");
    if (! fp) {
        fprintf(stderr, "Can't open the file.\r\n");
        return -1;
    }

    while ((line = fgets(buf, sizeof(buf), fp)) != NULL) {
        len = strlen(line);
        MEM_ReplaceChar(line, len, '\n', ';');
        if (Socket_WriteUntilFinish(fd, (void*)line, len, 0) < 0) {
            ret = -1;
            break;
        }
    }

    fclose(fp);

    return ret;
}

/* pipecmdc [options] cmds */
int main(int argc, char **argv)
{
    char *cmd = NULL;
    char *socket_name = NULL;
    char *filename = NULL;
    int ret = 0;

    GETOPT2_NODE_S opts[] = {
        {'o', 'h', "help", 0, NULL, NULL, 0},
        {'o', 'n', "name", 's', &socket_name, "domain socket name", 0},
        {'o', 'r', "read", 's', &filename, "read file", 0},
        {'p', 0, "CMD", 's', &cmd, "Command line", 0},
        {0}
    };

    if (BS_OK != GETOPT2_Parse(argc, argv, opts)) {
        _pipecmdc_help(opts);
        return -1;
    }

    if (GETOPT2_IsOptSetted(opts, 'h', NULL)) {
        _pipecmdc_help(opts);
        return 0;
    }

    if (socket_name == NULL) {
        socket_name = _PIPECMDC_FILE_NAME;
    }

    int fd = _pipecmdc_Connect(socket_name);
    if (fd < 0) {
        fprintf(stderr, "Can't connect server.\r\n");
        return -1;
    }

    if (filename) {
        ret = _pipecmdc_SendFile(fd, filename);
        if (ret != 0) {
            return -1;
        }
    } else if (cmd) {
        send(fd, cmd, strlen(cmd), 0);
    }

    send(fd, "\n", 1, 0);

    ret = _pipecmdc_Recv(fd);

    close(fd);

    return ret;
}

