/*================================================================
*   Created by LiXingang: 2018.12.17
*   Description: 硬盘的一些函数
*
================================================================*/
#include "bs.h"
#include "utl/hard_disk.h"

#ifdef IN_LINUX

#include <sys/ioctl.h>
#include <linux/hdreg.h>
#include <fcntl.h>
#include <linux/hdreg.h>
#include <scsi/scsi.h>
#include <scsi/sg.h>

#define MAX_RAID_SERIAL_LEN 1024 

static int hd_GetIdeDiskSn(OUT char *id, IN int max)
{
    int fd;
    struct hd_driveid hid;
    fd = open ("/dev/sda", O_RDONLY);
    if (fd < 0)
    {
        return -1;
    }
    if (ioctl (fd, HDIO_GET_IDENTITY, &hid) < 0)
    {
        close(fd);
        return -1;
    }
    close (fd);

    scnprintf (id, max, "%s", hid.serial_no);

    return 0;
}

int hd_scsi_get_serial(int fd, void *buf, size_t buf_len)
{
	unsigned char inq_cmd[] = {INQUIRY, 1, 0x80, 0, buf_len, 0};
	unsigned char sense[32];
	struct sg_io_hdr io_hdr;

	memset(&io_hdr, 0, sizeof(io_hdr));
	io_hdr.interface_id = 'S';
	io_hdr.cmdp = inq_cmd;
	io_hdr.cmd_len = sizeof(inq_cmd);
	io_hdr.dxferp = buf;
	io_hdr.dxfer_len = buf_len;
	io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
	io_hdr.sbp = sense;
	io_hdr.mx_sb_len = sizeof(sense);
	io_hdr.timeout = 5000;

	return ioctl(fd, SG_IO, &io_hdr);
}

int hd_scsi_read_serial(int fd, char *devname, char *serial)
{
	unsigned char scsi_serial[255];
	int rv;
	int rsp_len;
	int len;
	char *dest;
	char *src;
	char *rsp_buf;
	int i;

	memset(scsi_serial, 0, sizeof(scsi_serial));

	rv = hd_scsi_get_serial(fd, scsi_serial, sizeof(scsi_serial));
	if (rv != 0) {
		return rv;
	}

	rsp_len = scsi_serial[3];
	if (!rsp_len) {
		return 2;
	}

	rsp_buf = (char *) &scsi_serial[4];

	/* trim all whitespace and non-printable characters and convert
	 * ':' to ';'
	 */
	for (i = 0, dest = rsp_buf; i < rsp_len; i++) {
		src = &rsp_buf[i];
		if (*src > 0x20) {
			/* ':' is reserved for use in placeholder serial
			 * numbers for missing disks
			 */
			if (*src == ':')
				*dest++ = ';';
			else
				*dest++ = *src;
		}
	}
	len = dest - rsp_buf;
	dest = rsp_buf;

	/* truncate leading characters */
	if (len > MAX_RAID_SERIAL_LEN) {
		dest += len - MAX_RAID_SERIAL_LEN;
		len = MAX_RAID_SERIAL_LEN;
	}

	memset(serial, 0, MAX_RAID_SERIAL_LEN);
#ifndef __COVERITY__ 
	memcpy(serial, dest, len);
#endif
    serial[len] = '\0';

	return 0;
}

static int hd_scsi_get_disk_sn(OUT char *id, IN int max)
{
    int err;
	char serial[MAX_RAID_SERIAL_LEN + 1];
	int fd = open("/dev/sda", O_RDONLY);

    if (fd < 0) {
        return -1;
    }

    memset(serial, 0, sizeof(serial));
    err = hd_scsi_read_serial(fd, "/dev/sda", serial);
    if(err==0) {
        scnprintf (id, max, "%s", serial);
    }

	close(fd);

    return err;
}

/* 获取硬盘序列号 */
int HD_GetDiskSN(OUT char *id, IN int max)
{
    if (hd_GetIdeDiskSn(id, max) == 0) {
        return 0;
    }

    return hd_scsi_get_disk_sn(id, max);
}
#endif
