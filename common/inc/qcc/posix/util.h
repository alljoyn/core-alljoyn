#ifndef _ERUTIL_LINUX_H
#define _ERUTIL_LINUX_H
/**
 * @file
 *
 * This file provides platform specific macros for Linux
 */

/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/


#if defined(QCC_OS_DARWIN)
#include <machine/endian.h>
#include <libkern/OSByteOrder.h>

#define letoh16(_val) (OSSwapLittleToHostInt16(_val))
#define letoh32(_val) (OSSwapLittleToHostInt32(_val))
#define letoh64(_val) (OSSwapLittleToHostInt64(_val))

#define betoh16(_val) (OSSwapBigToHostInt16(_val))
#define betoh32(_val) (OSSwapBigToHostInt32(_val))
#define betoh64(_val) (OSSwapBigToHostInt64(_val))

#define htole16(_val) (OSSwapHostToLittleInt16(_val))
#define htole32(_val) (OSSwapHostToLittleInt32(_val))
#define htole64(_val) (OSSwapHostToLittleInt64(_val))

#define htobe16(_val) (OSSwapHostToBigInt16(_val))
#define htobe32(_val) (OSSwapHostToBigInt32(_val))
#define htobe64(_val) (OSSwapHostToBigInt64(_val))

/**
 * Swap bytes to convert endianness of a 16 bit integer
 */
#define EndianSwap16(_val) (OSSwapConstInt16(_val))

/**
 * Swap bytes to convert endianness of a 32 bit integer
 */
#define EndianSwap32(_val) (OSSwapConstInt32(_val))

/**
 * Swap bytes to convert endianness of a 64 bit integer
 */
#define EndianSwap64(_val) (OSSwapConstInt64(_val))

#else
#include <byteswap.h>
#include <endian.h>

#if !defined(QCC_OS_ANDROID)
#if !defined(betoh16)
/*
 * GLibC and BSD both define nice helper macros for converting between
 * big/little endian and the host machine's endian.  Unfortunately, for some
 * of those macros, they use slightly different names.  This unifies the names
 * to match BSD names (also used by Android'd Bionic).
 */
#if __BYTE_ORDER == __LITTLE_ENDIAN

#define letoh16(_val) (_val)
#define letoh32(_val) (_val)
#define letoh64(_val) (_val)

#define betoh16(_val) bswap_16(_val)
#define betoh32(_val) bswap_32(_val)
#define betoh64(_val) bswap_64(_val)

#else

#define letoh16(_val) bswap_16(_val)
#define letoh32(_val) bswap_32(_val)
#define letoh64(_val) bswap_64(_val)

#define betoh16(_val) (_val)
#define betoh32(_val) (_val)
#define betoh64(_val) (_val)

#endif
#endif

// Undefine GlibC's versions the macros to help prevent writing non-portable code.
#undef le16toh
#undef le32toh
#undef le64toh

#undef be16toh
#undef be32toh
#undef be64toh

// Again follow Android's Bionic example for compatability below
#define __swap16(_val) bswap_16(_val)
#define __swap32(_val) bswap_32(_val)
#define __swap64(_val) bswap_64(_val)
#endif

/**
 * Swap bytes to convert endianness of a 16 bit integer
 */
#define EndianSwap16(_val) (__swap16(_val))

/**
 * Swap bytes to convert endianness of a 32 bit integer
 */
#define EndianSwap32(_val) (__swap32(_val))

/**
 * Swap bytes to convert endianness of a 64 bit integer
 */
#define EndianSwap64(_val) (__swap64(_val))

#endif

/*
 * Make the target's endianness known to the rest of the code in portable manner.
 */
#if __BYTE_ORDER == __LITTLE_ENDIAN
/**
 * This target is little endian
 */
#define QCC_TARGET_ENDIAN QCC_LITTLE_ENDIAN
#else
/**
 * This target is big endian
 */
#define QCC_TARGET_ENDIAN QCC_BIG_ENDIAN
#endif

#define ER_DIR_SEPARATOR  "/"

#endif
