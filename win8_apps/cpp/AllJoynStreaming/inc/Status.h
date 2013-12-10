
/**
 * @file
 * This file contains an enumerated list values that QStatus can return
 *
 * Note: This file is generated during the make process.
 *
 * Copyright (c) 2009-2012, AllSeen Alliance. All rights reserved.
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
 */ 
#ifndef _STATUS_H
#define _STATUS_H

#ifndef ALLJOYN_DLLExport /* Used for extern C functions. Add __declspec(dllexport) when using MSVC */
#  if defined(_MSC_VER) /* MSVC compiler*/
#    define ALLJOYN_DLLExport __declspec(dllexport)
#  else /* compiler other than MSVC */
#    define ALLJOYN_DLLExport
#  endif /* Compiler type */
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Enumerated list of values QStatus can return
 */
typedef enum {
    ER_OK = 0x0 /**< Success. */,
    ER_FAIL = 0x1 /**< Generic failure. */,
    ER_UTF_CONVERSION_FAILED = 0x2 /**< Conversion between UTF bases failed. */,
    ER_BUFFER_TOO_SMALL = 0x3 /**< Not enough space in buffer for operation. */,
    ER_OS_ERROR = 0x4 /**< Underlying OS has indicated an error. */,
    ER_OUT_OF_MEMORY = 0x5 /**< Failed to allocate memory. */,
    ER_SOCKET_BIND_ERROR = 0x6 /**< Bind to IP address failed. */,
    ER_INIT_FAILED = 0x7 /**< Initialization failed. */,
    ER_WOULDBLOCK = 0x8 /**< An I/O attempt on non-blocking resource would block */,
    ER_NOT_IMPLEMENTED = 0x9 /**< Feature not implemented */,
    ER_TIMEOUT = 0xa /**< Operation timed out */,
    ER_SOCK_OTHER_END_CLOSED = 0xb /**< Other end closed the socket */,
    ER_BAD_ARG_1 = 0xc /**< Function call argument 1 is invalid */,
    ER_BAD_ARG_2 = 0xd /**< Function call argument 2 is invalid */,
    ER_BAD_ARG_3 = 0xe /**< Function call argument 3 is invalid */,
    ER_BAD_ARG_4 = 0xf /**< Function call argument 4 is invalid */,
    ER_BAD_ARG_5 = 0x10 /**< Function call argument 5 is invalid */,
    ER_BAD_ARG_6 = 0x11 /**< Function call argument 6 is invalid */,
    ER_BAD_ARG_7 = 0x12 /**< Function call argument 7 is invalid */,
    ER_BAD_ARG_8 = 0x13 /**< Function call argument 8 is invalid */,
    ER_INVALID_ADDRESS = 0x14 /**< Address is NULL or invalid */,
    ER_INVALID_DATA = 0x15 /**< Generic invalid data error */,
    ER_READ_ERROR = 0x16 /**< Generic read error */,
    ER_WRITE_ERROR = 0x17 /**< Generic write error */,
    ER_OPEN_FAILED = 0x18 /**< Generic open failure */,
    ER_PARSE_ERROR = 0x19 /**< Generic parse failure */,
    ER_END_OF_DATA = 0x1A /**< Generic EOD/EOF error */,
    ER_CONN_REFUSED = 0x1B /**< Connection was refused because no one is listening */,
    ER_BAD_ARG_COUNT = 0x1C /**< Incorrect number of arguments given to function call */,
    ER_WARNING = 0x1D /**< Generic warning */,
    ER_COMMON_ERRORS = 0x1000 /**< Error code block for the Common subsystem. */,
    ER_STOPPING_THREAD = 0x1001 /**< Operation interrupted by ERThread stop signal. */,
    ER_ALERTED_THREAD = 0x1002 /**< Operation interrupted by ERThread alert signal. */,
    ER_XML_MALFORMED = 0x1003 /**< Cannot parse malformed XML */,
    ER_AUTH_FAIL = 0x1004 /**< Authentication failed */,
    ER_AUTH_USER_REJECT = 0x1005 /**< Authentication was rejected by user */,
    ER_NO_SUCH_ALARM = 0x1006 /**< Attempt to reference non-existent timer alarm */,
    ER_TIMER_FALLBEHIND = 0x1007 /**< A timer thread is missing scheduled alarm times */,
    ER_SSL_ERRORS = 0x1008 /**< Error code block for SSL subsystem */,
    ER_SSL_INIT = 0x1009 /**< SSL initialization failed. */,
    ER_SSL_CONNECT = 0x100a /**< Failed to connect to remote host using SSL */,
    ER_SSL_VERIFY = 0x100b /**< Failed to verify identity of SSL destination */,
    ER_EXTERNAL_THREAD = 0x100c /**< Operation not supported on external thread wrapper */,
    ER_CRYPTO_ERROR = 0x100d /**< Non-specific error in the crypto subsystem */,
    ER_CRYPTO_TRUNCATED = 0x100e /**< Not enough room for key */,
    ER_CRYPTO_KEY_UNAVAILABLE = 0x100f /**< No key to return */,
    ER_BAD_HOSTNAME = 0x1010 /**< Cannot lookup hostname */,
    ER_CRYPTO_KEY_UNUSABLE = 0x1011 /**< Key cannot be used */,
    ER_EMPTY_KEY_BLOB = 0x1012 /**< Key blob is empty */,
    ER_CORRUPT_KEYBLOB = 0x1013 /**< Key blob is corrupted */,
    ER_INVALID_KEY_ENCODING = 0x1014 /**< Encoded key is not valid */,
    ER_DEAD_THREAD = 0x1015 /**< Operation not allowed thread is dead */,
    ER_THREAD_RUNNING = 0x1016 /**< Cannot start a thread that is already running */,
    ER_THREAD_STOPPING = 0x1017 /**< Cannot start a thread that is already stopping */,
    ER_BAD_STRING_ENCODING = 0x1018 /**< Encoded string did not have the expected format or contents */,
    ER_CRYPTO_INSUFFICIENT_SECURITY = 0x1019 /**< Crypto algorithm parameters do not provide sufficient security */,
    ER_CRYPTO_ILLEGAL_PARAMETERS = 0x101a /**< Crypto algorithm parameter value is illegal */,
    ER_CRYPTO_HASH_UNINITIALIZED = 0x101b /**< Cryptographic hash function must be initialized */,
    ER_THREAD_NO_WAIT = 0x101c /**< Thread cannot be blocked by a WAIT or SLEEP call */,
    ER_TIMER_EXITING = 0x101d /**< Cannot add an alarm to a timer that is exiting */,
    ER_INVALID_GUID = 0x101e /**< String is not a hex encoded GUID string */,
    ER_THREADPOOL_EXHAUSTED = 0x101f /**< A thread pool has reached its specified concurrency */,
    ER_THREADPOOL_STOPPING = 0x1020 /**< Cannot execute a closure on a stopping thread pool */,
    ER_NONE = 0xffff /**< No error code to report */,
    ER_MEDIA_ERRORS = 0xA000 /**< Streaming media errors block */,
    ER_MEDIA_STREAM_NOT_FOUND = 0xA001 /**< Media stream was not found */,
    ER_MEDIA_STREAM_OPEN = 0xA002 /**< Media stream is open */,
    ER_MEDIA_STREAM_CLOSED = 0xA003 /**< Media stream is closed */,
    ER_MEDIA_STREAM_EXISTS = 0xA004 /**< Media stream is already added */,
    ER_MEDIA_INVALID_MIME_TYPE = 0xA005 /**< Unexpected mimetype for the media */,
    ER_MEDIA_STREAM_ALREADY_STARTED = 0xA006 /**< Stream is already running */,
    ER_MEDIA_NO_STREAMS_TO_PLAY = 0xA007 /**< There are no streams to play */,
    ER_MEDIA_NO_STREAMS_TO_PAUSE = 0xA008 /**< There are no streams to pause */,
    ER_MEDIA_STREAM_OPEN_FAILED = 0xA009 /**< Media stream failed to open */,
    ER_MEDIA_STREAM_BUSY = 0xA00A /**< Operation cannot be performed because stream is busy */,
    ER_MEDIA_SEEK_FAILED = 0xA00B /**< Attempt to seek (rewind, fast-forward) in a media stream failed */,
    ER_MEDIA_HTTPSTREAMER_ALREADY_STARTED = 0xA00C /**< The HTTP Streamer has been started */
} QStatus;

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif
