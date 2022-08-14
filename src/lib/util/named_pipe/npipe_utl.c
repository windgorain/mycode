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
	int					fd, len, err, rval;
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
		rval = -2;
		goto errout;
	}

//    chmod(name, S_IRWXO);

	return(fd);

errout:
	err = errno;
	close(fd);
	errno = err;
	return(rval);
}

int NPIPE_Listen(IN CHAR *name)
{
	int					fd, len, err, rval;
	struct sockaddr_un	un;

	/* create a UNIX domain stream socket */
	if ((fd = Socket_Create(AF_UNIX, SOCK_STREAM)) < 0)
		return(-1);

	unlink(name);	/* in case it already exists */

	/* fill in socket address structure */
	memset(&un, 0, sizeof(un));
	un.sun_family = AF_UNIX;
	TXT_Strlcpy(un.sun_path, name, sizeof(un.sun_path));
	len = BS_OFFSET(struct sockaddr_un, sun_path) + strlen(name);

	/* bind the name to the descriptor */
	if (bind(fd, (struct sockaddr *)&un, len) < 0) {
		rval = -2;
		goto errout;
	}

	if (listen(fd, 10) < 0) {	/* tell kernel we're a server */
		rval = -3;
		goto errout;
	}

//    chmod(name, S_IRWXO);

	return(fd);

errout:
	err = errno;
	close(fd);
	errno = err;
	return(rval);
}

int NPIPE_Accept(int listenfd, OUT uid_t *uidptr)
{
	int					clifd, len, err, rval;
	time_t				staletime;
	struct sockaddr_un	un;
	struct stat			statbuf;

	len = sizeof(un);
	if ((clifd = Socket_Accept(listenfd, (struct sockaddr *)&un, &len)) < 0)
	{
		return(-1);		/* often errno=EINTR, if signal caught */
	}

    if (len >= sizeof(un)) {
        close(clifd);
        return -1;
    }

	/* obtain the client's uid from its calling address */
	len -= BS_OFFSET(struct sockaddr_un, sun_path); /* len of pathname */
	un.sun_path[len] = 0;			/* null terminate */

	if (stat(un.sun_path, &statbuf) < 0) {
		rval = -2;
		goto errout;
	}
#ifdef	S_ISSOCK	/* not defined for SVR4 */
	if (S_ISSOCK(statbuf.st_mode) == 0) {
		rval = -3;		/* not a socket */
		goto errout;
	}
#endif
	if ((statbuf.st_mode & (S_IRWXG | S_IRWXO)) ||
		(statbuf.st_mode & S_IRWXU) != S_IRWXU) {
		  rval = -4;	/* is not rwx------ */
		  goto errout;
	}

	staletime = time(NULL) - 30;
	if (statbuf.st_atime < staletime ||
		statbuf.st_ctime < staletime ||
		statbuf.st_mtime < staletime) {
		  rval = -5;	/* i-node is too old */
		  goto errout;
	}

	if (uidptr != NULL)
		*uidptr = statbuf.st_uid;	/* return uid of caller */

	unlink(un.sun_path);		/* we're done with pathname now */
	return(clifd);

errout:
	err = errno;
	close(clifd);
	errno = err;
	return(rval);
}

INT NPIPE_Connect(const char *name)
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
	len = BS_OFFSET(struct sockaddr_un, sun_path) + strlen(un.sun_path);

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
	TXT_Strlcpy(un.sun_path, (void *)name, sizeof(un.sun_path));
	len = BS_OFFSET(struct sockaddr_un, sun_path) + strlen(name);
	if (connect(fd, (struct sockaddr *)&un, len) < 0) {
		rval = -4;
		goto errout;
	}
	return(fd);

errout:
	err = errno;
	close(fd);
	errno = err;
	return(rval);
}

#endif


