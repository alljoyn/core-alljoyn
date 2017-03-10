#ifndef _QCC_PLATFORM_TYPES_H
#define _QCC_PLATFORM_TYPES_H
/**
 * @file
 *
 * This file defines basic types for posix platforms.
 */

/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#if defined(QCC_OS_DARWIN)
#include <AvailabilityMacros.h>
#endif
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>

#ifdef __cplusplus
namespace qcc {
typedef int SocketFd;      /**< Socket file descriptor type. */
typedef int UARTFd;      /**< UART file descriptor type. */
}
#else
typedef int qcc_SocketFd; /**< Socket file descriptor type. */
typedef int qcc_UARTFd; /**< UART file descriptor type. */
#endif

#ifndef PRIuSIZET
/**
 * Platform pointer-sized unsigned integer (i.e., size_t)
 */
#define PRIuSIZET "zu"
/**
 * Platform pointer-sized signed integer (i.e., ssize_t)
 */
#define PRIiSIZET "zd"
#endif

/**
 * Zero-terminated strings.
 */
typedef char*       AJ_PSTR;
typedef const char* AJ_PCSTR;

#endif
