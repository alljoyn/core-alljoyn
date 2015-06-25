/**
 * @file
 *
 * This file just wraps platform specific header files that define the thread
 * abstraction interface.
 */

/******************************************************************************
 *
 *
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
#ifndef _OS_QCC_THREAD_H
#define _OS_QCC_THREAD_H

#include <qcc/platform.h>

#include <pthread.h>
#include <sys/select.h>

/* Linux does not use Windows conventions for STDCALL and CDECL */
#define STDCALL

namespace qcc {

typedef pthread_t ThreadHandle;        ///< Linux uses pthreads under the hood.
typedef ThreadHandle ThreadId;         ///< Linux thread IDs are the thread handles.
typedef void* ThreadInternalReturn;    ///< Return type for pthreads.
}

#endif
