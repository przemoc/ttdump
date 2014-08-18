/*
 *  tuntap.h  --  TUN/TAP interface
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

#ifndef TUNTAP_H_
#define TUNTAP_H_


/** Maximal TUN/TAP device name including trailing NUL. */
#define TT_DEVNAMSIZ 16


/** TUN device. */
#define TT_FLG_TUN          (1 <<  1)
/** Prepend TUN frames with flags (2 bytes) and proto (2 bytes). */
#define TT_FLG_TUN_PKTINFO  (1 <<  2)
/** TAP device. */
#define TT_FLG_TAP          (1 <<  3)
/** Set the close-on-exec flag on the endpoint. */
#define TT_FLG_CLOEXEC      (1 << 12)
/** Set non-blocking status flag on the endpoint. */
#define TT_FLG_NONBLOCK     (1 << 13)
/** Bring device up. */
#define TT_FLG_IFUP         (1 << 15)


#define TT_ERR_UNKNOWNFLAG   -2  /**< Some flag is unknown. */
#define TT_ERR_WRONGFLAGS    -3  /**< Given flags are incompatible. */
#define TT_ERR_OPEN          -4  /**< Opening TUN/TAP control file failed. */
#define TT_ERR_SETCLOEXEC    -5  /**< Setting close-on-exec flag failed. */
#define TT_ERR_SETIF         -6  /**< Setting TUN/TAP device name failed. */
#define TT_ERR_NEWSOCKET     -7  /**< Creating new socket failed. */
#define TT_ERR_GETFLAGS      -8  /**< Getting device flags failed. */
#define TT_ERR_BRINGIFUP     -9  /**< Bringing device up failed. */
#define TT_ERR_SETNONBLOCK  -10  /**< Setting non-blocking flag failed. */


/**
 * Creates or attaches to a TUN/TAP device.
 * If format string is used in dev, like: "tap%d", then dev is overwritten
 * with the real device name.
 */
int tt_open(char *dev, int flags);


#endif /* TUNTAP_H_ */
