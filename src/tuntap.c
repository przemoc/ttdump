/*
 *  tuntap.c  --  TUN/TAP interface
 *
 * Copyright (c) 2014 Przemyslaw Pawelczyk <przemoc@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "tuntap.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h> /* must precede <linux/if.h> to avoid errors */
#include <linux/if.h>
#include <linux/if_tun.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


/** All available tuntap flags. */
#define TT_ALL_FLAGS \
	(0 | TT_FLG_TUN | TT_FLG_TUN_PKTINFO | TT_FLG_TAP \
	   | TT_FLG_CLOEXEC | TT_FLG_NONBLOCK | TT_FLG_IFUP)


int tt_open(char *dev, int flags)
{
	struct ifreq ifr;
	int fd, res, errsv;
	int sk = -1;
	char *ttdev = "/dev/net/tun";

	if (flags & ~TT_ALL_FLAGS) {
		errno = EINVAL;
		return TT_ERR_UNKNOWNFLAG;
	}

	if ((flags & TT_FLG_TAP) && (flags & TT_FLG_TUN_PKTINFO)) {
		errno = EINVAL;
		return TT_ERR_WRONGFLAGS;
	}

	fd = open(ttdev, O_RDWR);
	if (fd < 0)
		return TT_ERR_OPEN;

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = 0
	                | !!(flags & TT_FLG_TUN)         * IFF_TUN
	                |  !(flags & TT_FLG_TUN_PKTINFO) * IFF_NO_PI
	                | !!(flags & TT_FLG_TAP)         * IFF_TAP
	                ;

	if (*dev)
		strncpy(ifr.ifr_name, dev, IFNAMSIZ);

	res = ioctl(fd, TUNSETIFF, (void *)&ifr);
	if (res < 0) {
		res = TT_ERR_SETIF;
		goto tt_open_err;
	}

	strcpy(dev, ifr.ifr_name);

	if (flags & TT_FLG_IFUP) {
		/* Yes, we do need dummy socket to perform below ioctl()s... */
		int sk = socket(PF_INET, SOCK_DGRAM, 0);
		if (sk < 0) {
			res = TT_ERR_NEWSOCKET;
			goto tt_open_err;
		}
		res = ioctl(sk, SIOCGIFFLAGS, &ifr);
		if (res < 0) {
			res = TT_ERR_GETFLAGS;
			goto tt_open_err;
		}
		if ((ifr.ifr_flags & IFF_UP) == 0) {
			ifr.ifr_flags |= IFF_UP;
			res = ioctl(sk, SIOCSIFFLAGS, &ifr);
			if (res == -1) {
				res = TT_ERR_BRINGIFUP;
				goto tt_open_err;
			}
		}
		close(sk);
		sk = -1;
	}

	if (flags & TT_FLG_CLOEXEC) {
		res = fcntl(fd, F_SETFD, fcntl(fd, F_GETFD, 0) | FD_CLOEXEC);
		if (res == -1) {
			res = TT_ERR_SETCLOEXEC;
			goto tt_open_err;
		}
	}

	if (flags & TT_FLG_NONBLOCK) {
		res = fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
		if (res == -1) {
			res = TT_ERR_SETNONBLOCK;
			goto tt_open_err;
		}
	}


	return fd;

tt_open_err:
	errsv = errno;
	if (sk >= 0)
		close(sk);
	close(fd);
	errno = errsv;

	return res;
}
