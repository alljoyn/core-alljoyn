#ifndef _QCC_UTIL_H
#define _QCC_UTIL_H
/**
 * @file
 *
 * This file provides some useful utility macros and wrappers around system APIs
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

#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/Environ.h>
#include <list>
#include <Status.h>


/*
 * Platform util.h will define QCC_TARGET_ENDIAN to one of these.  This will
 * allow for compile time determination of the target system's endianness.
 */
#define QCC_LITTLE_ENDIAN 1234
#define QCC_BIG_ENDIAN    4321


/*
 * Include platform-specific utility macros
 */
#if defined(QCC_OS_GROUP_POSIX)
#include <qcc/posix/util.h>
#elif defined(QCC_OS_GROUP_WINDOWS)
#include <qcc/windows/util.h>
#else
#error No OS GROUP defined.
#endif


/**
 * Returns the size of a statically allocated array
 */
#define ArraySize(a)  (sizeof(a) / sizeof(a[0]))


/**
 * Number of pad bytes needed to align to a specified byte boundary.
 *
 * @param p  The pointer to align
 * @param b  The byte boundary to align it on (b must be a power of 2)
 */
#define PadBytes(p, b) (((b) - reinterpret_cast<size_t>(p)) & ((b) - 1))

/**
 * Return a pointer aligned (up) to a specified byte boundary
 *
 * @param p  The pointer to align
 * @pram b   The byte boundary to align it on (b must be a power of 2)
 */
#define AlignPtr(p, b) ((p) + PadBytes(p, b))

/**
 * Return the number of array elements needed to store a number of bytes
 *
 * @param bytes  Required number of bytes in the array
 * @param type   Type of the array elements
 */
#define RequiredArrayLength(bytes, type) (((bytes) + sizeof(type) - 1) / sizeof(type))

/**
 * Return the number of bytes required to store a number of bits
 * (rounds up to the next byte).
 *
 * @param bits Number of bits
 */
#define BitlenToBytelen(bits) (((bits) + 7) / 8)

namespace qcc {
/**
 * Returns a hash of C-style string
 *
 * @return Hash value
 */
inline size_t hash_string(const char* __s) {
    unsigned long __h = 0;
    for (; *__s; ++__s)
        __h = 5 * __h + *__s;

    return size_t(__h);
}

/**
 * The enum defining the high level operating system on the device.
 */
typedef enum _OSType {

    /*Invalid*/
    NONE = 0,

    /*Android*/
    ANDROID_OS,

    /*Windows*/
    WINDOWS_OS,

    /*Darwin*/
    DARWIN_OS,

    /*Linux*/
    LINUX_OS,

} OSType;

/**
 * @brief Get the OS type.
 */
OSType GetSystemOSType(void);

/**
 * Return an 8 bit random number.
 *
 * @return An 8 bit random number
 */
uint8_t Rand8();

/**
 * Return a 16 bit random number.
 *
 * @return A 16 bit random number
 */
uint16_t Rand16();

/**
 * Return a cryptographically strong 32 bit random number
 *
 * @return A 32 bit random number
 */
uint32_t Rand32();

/**
 * Return a cryptographically strong 64 bit random number
 *
 * @return A 64 bit random number
 */
uint64_t Rand64();

/**
 * Clear memory even with compiler optimizations
 */
void ClearMemory(void* s, size_t n);

/**
 * Return the Process ID as an unsigned 32 bit integer.
 *
 * @return The 32 bit Process ID
 */
uint32_t GetPid();

/**
 * Return the User ID as an unsigned 32 bit integer.
 *
 * @return The 32 bit User ID
 */
uint32_t GetUid();

/**
 * Return the Group ID as an unsigned 32 bit integer.
 *
 * @return The 32 bit User ID
 */
uint32_t GetGid();

/**
 * Return the User ID of the named user as an unsigned 32 bit integer.
 *
 * @param name  Username to lookup.
 */
uint32_t GetUsersUid(const char* name);

/**
 * Return the Group ID of the named user as an unsigned 32 bit integer.
 *
 * @param name  Username to lookup.
 */
uint32_t GetUsersGid(const char* name);

/**
 * Return the home directory of the calling user
 */
qcc::String GetHomeDir();

/**
 * Return a string with random, filename safe characters.
 *
 * @param prefix    [optional] resulting string will start with this.
 * @param len       [optional] number of charaters to generate.
 *
 * @return  The string with random characters.
 */
qcc::String RandomString(const char* prefix = NULL, size_t len = 10);


/**
 * Container type for directory listing results.
 */
typedef std::list<qcc::String> DirListing;

/**
 * Get a list of files and subdirectories in the specified path.
 *
 * @param path      The path from which the listing will be gathered.
 * @param listing   [OUT] Collection of filenames in path.
 *
 * @return  ER_OK if directory listing was gathered without problems.
 */
QStatus GetDirListing(const char* path, DirListing& listing);


/**
 * Container type for arguments to a program to be executed.
 */
typedef std::list<qcc::String> ExecArgs;

/**
 * Execute the specified program in a separate process with the specified
 * arguments.
 *
 * @param exec  Program to be executed.
 * @param args  Arguments to be passed to the program.
 * @param envs  Environment variables defining the envrionment the program will run in.
 *
 * @return  ER_OK if program was launched successfully.
 */
QStatus Exec(const char* exec, const ExecArgs& args, const qcc::Environ& envs);

/**
 * Execute the specified program in a separate process with the specified
 * arguments as a different user.
 *
 * @param user  user ID of the user to run as.
 * @param exec  Program to be executed.
 * @param args  Arguments to be passed to the program.
 * @param envs  Environment variables defining the envrionment the program will run in.
 *
 * @return  ER_OK if program was launched successfully.
 */
QStatus ExecAs(const char* user, const char* exec, const ExecArgs& args, const qcc::Environ& envs);


/**
 * Computes crc on a buffer using caller's running CRC as a base.
 *
 * @param buffer      Pointer to data bytes.
 * @param bufLen      Number of bytes in buffer.
 * @param runningCrc  [IN/OUT] Initial CRC16 value on input, final CRC16 value on output.
 */
void CRC16_Compute(const uint8_t* buffer, size_t bufLen, uint16_t*runningCrc);

/**
 * Resolves a hostname to its packed address representation.
 *
 * @param hostname  hostname to resolve.
 * @param addr      Buffer to receive the packed address.
 * @param addrSize  Size of the buffer specified in addr.
 * @param addrLen   The number of bytes copied into the addr buffer.
 * @param timeoutMs The timeout for hostname resolution.
 *
 * @return  ER_OK if conversion was successful.
 */
QStatus ResolveHostName(qcc::String hostname, uint8_t addr[], size_t addrSize, size_t& addrLen, uint32_t timeoutMs);

}
#endif
