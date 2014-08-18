/*
 *  ttdump.c  --  TUN/TAP dump
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
#include "dumphex.h"
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_FRAME_SIZE (9126 + 14)  /* jumbo + header w/o FCS */


const char ttdump_usage[] =
	"Usage: %s DEVNAME {tun|tap}\n"
	"Dump packets/frames reaching given TUN/TAP interface.\n"
	;


int print_err(const char *fname, int ret)
{
	return fprintf(stderr, "%s() returned %d, errno = %d (%s).\n",
	               fname, ret, errno, strerror(errno));
}


int check_and_parse(int argc, char *argv[], char *devnam, int *mode)
{
	if (argc == 1) {
		printf(ttdump_usage, argv[0]);
		return -1;
	}

	if (argc != 3) {
		fprintf(stderr, "You have to specify TUN/TAP device name and mode!\n");
		return -2;
	}

	*mode =   !strcmp(argv[2], "tun") * TT_FLG_TUN
	        | !strcmp(argv[2], "tap") * TT_FLG_TAP;

	if ((unsigned)(strlen(argv[1]) - 1) >= (TT_DEVNAMSIZ - 1) || !*mode) {
		fprintf(stderr, "Please give proper TUN/TAP device name and mode!\n");
		return -3;
	}

	strncpy(devnam, argv[1], TT_DEVNAMSIZ);

	return 0;
}


int main(int argc, char *argv[])
{
	int fd, res, mode;
	char devnam[TT_DEVNAMSIZ];

	res = check_and_parse(argc, argv, devnam, &mode);
	if (res < 0)
		return -1 - res;

	fd = tt_open(devnam,   mode
	                     | TT_FLG_CLOEXEC | TT_FLG_NONBLOCK | TT_FLG_IFUP);
	if (fd < 0) {
		print_err("tt_open", fd);
		return 3;
	}

	struct pollfd pfd = { fd, POLLIN, 0 };
	char framebuf[MAX_FRAME_SIZE];
	char *hexbuf = NULL;

	for (;;) {
		res = poll(&pfd, 1, -1);
		if (res < 0) {
			print_err("poll", res);
			break;
		}

		res = read(fd, framebuf, sizeof(framebuf));
		if (res == -1) {
			if (errno == EINTR)
				continue;
			print_err("read", res);
			break;
		}
		if (res == 0) {
			fprintf(stderr, "read() returned 0.\n");
			break;
		}

		dumphex(&hexbuf, framebuf, res,
		        DHEX_SHOWADDR | DHEX_RELADDR | DHEX_SHOWCHAR, NULL);
		puts(hexbuf);
	}

	close(fd);

	return 0;
}
