/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2011-7-12
* Description: Named pipe
* History:     
******************************************************************************/

#include "bs.h"
#include "utl/txt_utl.h"
#include "utl/socket_utl.h"

#ifdef IN_UNIXLIKE

#include <sys/un.h>

#include "utl/npipe_utl.h"

int NPIPE_OpenDgram(IN CHAR *name)
{
	int					fd, len;
	struct sockaddr_un	un;

	/* create a UNIX domain stream socket */
	if ((fd = Socket_Create(AF_UNIX, SOCK_DGRAM)) < 0)
		return(-1);

	unlink(name);	/* in case it already exists */

	/* fill in socket address structure */
	memset(&un, 0, sizeof(un));
	un.sun_family = AF_UNIX;
	TXT_Strlcpy(un.sun_path, name, sizeof(un.sun_path));
	len = BS_OFFSET(struct sockaddr_un, sun_path) + strlen(name);

	/* bind the name to the descriptor */
	if (bind(fd, (struct sockaddr *)&un, len) < 0) {
        close(fd);
        return -2;
	}

//    chmod(name, S_IRWXO);

	return(fd);
}

int NPIPE_OpenSeqpacket(IN CHAR *name)
{
	int					fd, len;
	struct sockaddr_un	un;

	/* create a UNIX domain stream socket */
	if ((fd = Socket_Create(AF_UNIX, SOCK_SEQPACKET)) < 0) {
        return -1;
    }

	unlink(name);	/* in case it already exists */

	/* fill in socket address structure */
	memset(&un, 0, sizeof(un));
	un.sun_family = AF_UNIX;
	TXT_Strlcpy(un.sun_path, name, sizeof(un.sun_path));
	len = BS_OFFSET(struct sockaddr_un, sun_path) + strlen(name);

	/* bind the name to the descriptor */
	if (bind(fd, (struct sockaddr *)&un, len) < 0) {
        close(fd);
        return -2;
	}

	if (listen(fd, 10) < 0) {	/* tell kernel we're a server */
        close(fd);
        return -3;
	}

//    chmod(name, S_IRWXO);

	return(fd);
}

int NPIPE_OpenStream(IN CHAR *name)
{
	int					fd, len;
	struct sockaddr_un	un;

	/* create a UNIX domain stream socket */
	if ((fd = Socket_Create(AF_UNIX, SOCK_STREAM)) < 0) {
        return -1;
    }

	unlink(name);	/* in case it already exists */

	/* fill in socket address structure */
	memset(&un, 0, sizeof(un));
	un.sun_family = AF_UNIX;
	TXT_Strlcpy(un.sun_path, name, sizeof(un.sun_path));
	len = BS_OFFSET(struct sockaddr_un, sun_path) + strlen(name);

	/* bind the name to the descriptor */
	if (bind(fd, (struct sockaddr *)&un, len) < 0) {
        close(fd);
        return -2;
	}

	if (listen(fd, 10) < 0) {	/* tell kernel we're a server */
        close(fd);
        return -3;
	}

//    chmod(name, S_IRWXO);

	return(fd);
}

int NPIPE_Accept(int listenfd)
{
	int					clifd, len;
	struct sockaddr_un	un;

	len = sizeof(un);
	if ((clifd = Socket_Accept(listenfd, (struct sockaddr *)&un, &len)) < 0) {
		return(-1);		/* often errno=EINTR, if signal caught */
	}

    if (len >= sizeof(un)) {
        close(clifd);
        return -1;
    }

	unlink(un.sun_path);		/* we're done with pathname now */

	return(clifd);
}

static int _npipe_connect(int type, const char *name)
{
	int					fd, len;
	struct sockaddr_un	un;

	/* create a UNIX domain stream socket */
	if ((fd = socket(AF_UNIX, type, 0)) < 0)
		return(-1);

	/* fill socket address structure with our address */
	memset(&un, 0, sizeof(un));
	un.sun_family = AF_UNIX;
	sprintf(un.sun_path, "/var/tmp/%05d", getpid());
	len = BS_OFFSET(struct sockaddr_un, sun_path) + strlen(un.sun_path);

	unlink(un.sun_path);		/* in case it already exists */
	if (bind(fd, (struct sockaddr *)&un, len) < 0) {
        close(fd);
        return -2;
	}
	if (chmod(un.sun_path, S_IRWXU) < 0) {
        close(fd);
        return -3;
	}

	/* fill socket address structure with server's address */
	memset(&un, 0, sizeof(un));
	un.sun_family = AF_UNIX;
	strlcpy(un.sun_path, (void *)name, sizeof(un.sun_path));
	len = BS_OFFSET(struct sockaddr_un, sun_path) + strlen(name);
	if (connect(fd, (struct sockaddr *)&un, len) < 0) {
        close(fd);
        return -4;
	}

	return(fd);
}

int NPIPE_ConnectStream(const char *name)
{
    return _npipe_connect(SOCK_STREAM, name);
}

int NPIPE_ConnectSeqpacket(const char *name)
{
    return _npipe_connect(SOCK_SEQPACKET, name);
}

#endif


