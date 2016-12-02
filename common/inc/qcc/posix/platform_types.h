#ifndef _QCC_PLATFORM_TYPES_H
#define _QCC_PLATFORM_TYPES_H
/**
 * @file
 *
 * This file defines basic types for posix platforms.
 */

/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

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