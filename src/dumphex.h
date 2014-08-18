/*
 *  dumphex.h
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

#ifndef DUMPHEX_H_
#define DUMPHEX_H_

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <inttypes.h>


#define DHEX_SHOWADDR (1 <<  0)
#define DHEX_RELADDR  (1 <<  1)
#define DHEX_SHOWCHAR (1 <<  2)


static inline int dumphex(char **dump, const char *ptr, size_t len,
                          unsigned flags, const char *prefix)
{
	const unsigned show_addr = !!(flags & DHEX_SHOWADDR);
	const unsigned rel_addr  = !!(flags & DHEX_RELADDR);
	const unsigned show_char = !!(flags & DHEX_SHOWCHAR);

	const int PREFIX_LEN = prefix ? strlen(prefix) : 0;
	const int MAX_LINE_LEN = !!PREFIX_LEN * (PREFIX_LEN + 1) +
	                         show_addr * (8 + !rel_addr * 8 /* addr */ + 2 /* margin */) +
	                         3 * 16 /* hex */ +
	                         show_char * (1 /* margin */ + 16 /* char */) +
	                         1 /* LF */;
	char buf[MAX_LINE_LEN + 1];
	const char * const orgptr = ptr;
	const char * const endptr = ptr + len;
	int pos, s, dpos = 0;

	*dump = (char *)realloc(*dump, ((len + 15) / 16) * MAX_LINE_LEN + 1);

	for (; ptr + 15 < endptr; ptr += 16) {
		pos = 0;
		if (PREFIX_LEN) {
			memcpy(buf + pos, prefix, PREFIX_LEN);
			pos += PREFIX_LEN;
			buf[pos++] = ' ';
		}
		if (show_addr)
			pos += sprintf(buf + pos, "%08"PRIx64"  ", (uint64_t)ptr - rel_addr * (uint64_t)orgptr);
		for (int i = 0; i < 16; i++) {
			sprintf(&buf[pos + i * 3], "%02x ", (unsigned)(uint8_t)ptr[i]);
			if (show_char)
				buf[pos + 16 * 3 + 1 + i] = isprint(ptr[i]) ? ptr[i] : '.';
		}
		if (show_char) {
			buf[pos + 16 * 3] = ' ';
			buf[pos + 16 * 3 + 1 + 16] = '\0';
		}
		s = strlen(buf);
		buf[s] = '\n';
		memcpy(&(*dump)[dpos], buf, s + 1);
		dpos += s + 1;
		(*dump)[dpos] = '\0';
	}

	if (ptr >= endptr)
		return dpos;

	pos = 0;
	if (PREFIX_LEN) {
		memcpy(buf + pos, prefix, PREFIX_LEN);
		pos += PREFIX_LEN;
		buf[pos++] = ' ';
	}
	if (show_addr)
		pos += sprintf(buf + pos, "%08"PRIx64"  ", (uint64_t)ptr - rel_addr * (uint64_t)orgptr);
	for (int i = 0; i < endptr - ptr; i++) {
		sprintf(&buf[pos + i * 3], "%02x ", (unsigned)(uint8_t)ptr[i]);
		if (show_char)
			buf[pos + 16 * 3 + 1 + i] = isprint(ptr[i]) ? ptr[i] : '.';
	}
	if (show_char) {
		for (int i = endptr - ptr; i < 16; i++) {
			buf[pos + i * 3 + 0] = ' ';
			buf[pos + i * 3 + 1] = ' ';
			buf[pos + i * 3 + 2] = ' ';
		}
		buf[pos + 16 * 3] = ' ';
		buf[pos + 16 * 3 + 1 + endptr - ptr] = '\0';
	}
	s = strlen(buf);
	buf[s] = '\n';
	memcpy(&(*dump)[dpos], buf, s + 1);
	dpos += s + 1;
	(*dump)[dpos] = '\0';

	return dpos;
}


#endif // DUMPHEX_H_
