/******************************************************************************
 * Copyright (c) 2014 AllSeen Alliance. All rights reserved.
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
#include <gtest/gtest.h>

#include <qcc/Thread.h>

using namespace std;
using namespace qcc;

#if defined(QCC_OS_GROUP_POSIX)
static void* ExternalThread(void*) {
#elif defined(QCC_OS_GROUP_WINDOWS)
static unsigned __stdcall ExternalThread(void*) {
#endif
    Thread* thread = Thread::GetThread();
    (void)thread;  // Suppress unused warning given by G++.
    return NULL;
}

/*
 * Test this with valgrind to see that no memory leaks occur with
 * external thread objects.
 */
TEST(ThreadTest, CleanExternalThread) {
#if defined(QCC_OS_GROUP_POSIX)
    pthread_t tid;
    ASSERT_EQ(0, pthread_create(&tid, NULL, ExternalThread, NULL));
    ASSERT_EQ(0, pthread_join(tid, NULL));
#elif defined(QCC_OS_GROUP_WINDOWS)
    uintptr_t handle = _beginthreadex(NULL, 0, ExternalThread, NULL, 0, NULL);
    ASSERT_NE(0, handle);
    ASSERT_EQ(WAIT_OBJECT_0, WaitForSingleObject(reinterpret_cast<HANDLE>(handle), INFINITE));
    ASSERT_NE(0, CloseHandle(reinterpret_cast<HANDLE>(handle)));
#endif
}
