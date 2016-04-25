#ifndef _QCC_UTIL_WINDOWS_H
#define _QCC_UTIL_WINDOWS_H
/**
 * @file
 *
 * This file provides platform specific macros for Windows
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

/* Windows only runs on little endian machines (for now?) */

#include <stdlib.h>

/**
 * This target is little endian
 */
#define QCC_TARGET_ENDIAN QCC_LITTLE_ENDIAN

/*
 * Define some endian conversion macros to be compatible with posix macros.
 * Macros with the _same_ names are available on BSD (and Android Bionic)
 * systems (and with _similar_ names on GLibC based systems).
 *
 * Don't bother with a version of those macros for big-endian targets for
 * Windows.
 */

#define htole16(_val) (_val)
#define htole32(_val) (_val)
#define htole64(_val) (_val)

#define htobe16(_val) _byteswap_ushort(_val)
#define htobe32(_val) _byteswap_ulong(_val)
#define htobe64(_val) _byteswap_uint64(_val)

#define letoh16(_val) (_val)
#define letoh32(_val) (_val)
#define letoh64(_val) (_val)

#define betoh16(_val) _byteswap_ushort(_val)
#define betoh32(_val) _byteswap_ulong(_val)
#define betoh64(_val) _byteswap_uint64(_val)


/**
 * Swap bytes to convert endianness of a 16 bit integer
 */
#define EndianSwap16(_val) (_byteswap_ushort(_val))

/**
 * Swap bytes to convert endianness of a 32 bit integer
 */
#define EndianSwap32(_val) (_byteswap_ulong(_val))

/**
 * Swap bytes to convert endianness of a 64 bit integer
 */
#define EndianSwap64(_val) (_byteswap_uint64(_val))


#define ER_DIR_SEPARATOR  "\\"

/**
 * Non-standard/non-compliant snprintf implementation, based on _vsnprintf.
 *
 * @remark This function always adds a zero character string terminator,
 *         even when the output is truncated due to the output buffer
 *         being too small. However, it is non-compliant - mainly because
 *         it returns -1 when the output buffer is too small, rather than
 *         returning the required length of the output buffer as required
 *         by the C99 standard.
 *
 * @param[out] buffer    Storage location for the output.
 * @param[in]  count     Maximum number of characters to store.
 * @param[in]  format    Format-control string.
 * @param[in]  ...       String format parameters.
 *
 * @return
 *      - The length of the output buffer, not including the zero terminator, on success
 *      - Value -1 if the output has been truncated
 */
int32_t AJ_snprintf(char* buffer, size_t count, const char* format, ...);

#if ((_MSC_VER <= 1800) || defined(DO_SNPRINTF_MAPPING))
#define snprintf AJ_snprintf
#endif

#endif
