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

#define MAX_IF_COUNT 16

#define _str(x) #x
#define str(x) _str(x)


const char ttdump_usage[] =
	"Usage: %s [-c] DEVNAME... {tun|tap}\n"
	"\n"
	"Dump packets/frames reaching given TUN/TAP interfaces "
	"(up to " str(MAX_IF_COUNT) ").\n"
	"If -c option is given, then data received on some interface is sent to "
	"the next one (which is the same one if there is only one interface).\n"
	;


int print_err(const char *fname, int ret)
{
	return fprintf(stderr, "%s() returned %d, errno = %d (%s).\n",
	               fname, ret, errno, strerror(errno));
}


int print_err_prefix(const char *prefix, const char *fname, int ret)
{
	return fprintf(stderr, "%s %s() returned %d, errno = %d (%s).\n",
	               prefix, fname, ret, errno, strerror(errno));
}


int check_and_parse(int argc, char *argv[], char *devnams,
                    int *mode, int *copy)
{
	int pos = 1;

	if (argc == 1) {
		printf(ttdump_usage, argv[0]);
		return -1;
	}

	if (!strcmp(argv[1], "-c")) {
		pos++;
		*copy = 1;
	}

	if (argc < pos + 2) {
		fprintf(stderr, "You have to specify TUN/TAP device name and mode!\n");
		return -2;
	}

	if (argc - pos > MAX_IF_COUNT) {
		fprintf(stderr, "You specified too many TUN/TAP devices!\n");
		return -3;
	}

	*mode =   !strcmp(argv[argc - 1], "tun") * TT_FLG_TUN
	        | !strcmp(argv[argc - 1], "tap") * TT_FLG_TAP;

	if ((unsigned)(strlen(argv[1]) - 1) >= (TT_DEVNAMSIZ - 1) || !*mode) {
		fprintf(stderr, "Please give proper TUN/TAP device name and mode!\n");
		return -4;
	}

	for (int i = 0; i < argc - pos - 1; i++)
		strncpy(devnams + i * TT_DEVNAMSIZ, argv[pos + i], TT_DEVNAMSIZ);

	return argc - pos - 1;
}


int main(int argc, char *argv[])
{
	int fds, res, mode, copy = 0;
	struct pollfd pfd[MAX_IF_COUNT] = { { 0 } };
	char devnams[MAX_IF_COUNT][TT_DEVNAMSIZ] = { { 0 } };

	fds = check_and_parse(argc, argv, devnams[0], &mode, &copy);
	if (fds < 0)
		return -1 - fds;

	for (int i = 0; i < fds; i++) {
		pfd[i].events = POLLIN;
		printf("Opening %s...\n", devnams[i]);
		pfd[i].fd = tt_open(devnams[i],   mode
		                                | TT_FLG_CLOEXEC | TT_FLG_IFUP);
		if (pfd[i].fd < 0) {
			print_err("tt_open", pfd[i].fd);
			return 4;
		}
		printf("%s opened.\n", devnams[i]);
	}

	puts("");

	char framebuf[MAX_FRAME_SIZE];
	char *hexbuf = NULL;

	for (int i = 0;;) {
		res = poll(pfd, fds, -1);
		if (res < 0) {
			print_err("poll", res);
			break;
		}

		for (i = 0; i < fds; i++) {
			if (!pfd[i].revents)
				continue;

			res = read(pfd[i].fd, framebuf, sizeof(framebuf));
			if (res == -1) {
				if (errno == EINTR)
					continue;
				print_err_prefix(devnams[i], "read", res);
				i = -1;
				break;
			}
			if (res == 0) {
				fprintf(stderr, "%s read() returned 0.\n", devnams[i]);
				i = -1;
				break;
			}

			dumphex(&hexbuf, framebuf, res,
			        DHEX_SHOWADDR | DHEX_RELADDR | DHEX_SHOWCHAR, devnams[i]);
			puts(hexbuf);

			if (copy) {
				res = write(pfd[(i + 1) % fds].fd, framebuf, res);
				if (res == -1 && errno != EINTR) {
					print_err_prefix(devnams[(i + 1) % fds], "write", res);
					i = -1;
					break;
				}

				printf("%s sent %d bytes.\n\n", devnams[(i + 1) % fds], res);
			}
		}

		if (i < 0)
			break;
	}

	while (--fds >= 0)
		close(pfd[fds].fd);

	return 0;
}
