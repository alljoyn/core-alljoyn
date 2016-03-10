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

/* These tests apply only to CNG on Windows. */
#ifdef CRYPTO_CNG

#include <gtest/gtest.h>
#include <qcc/Crypto.h>
#include "../crypto/Crypto.h"
#include <qcc/CngCache.h>
#include <qcc/Thread.h>
#include <qcc/Util.h>

#include <memory>
#include <vector>

using namespace qcc;

/*
 * These tests have multiple threads that all attempt to initialize provider handles at the same
 * time. No matter what, when everything is done, all the threads should come away with the
 * same provider handles as all the other threads.
 */

static const int trials = 20;
static const int numThreads = 100;

typedef struct {
    BCRYPT_ALG_HANDLE handles[CngCache::ALGORITHM_COUNT][2];
} MultipleOpenHashHandleTestResult;

static ThreadReturn STDCALL MultipleOpenHashHandleThreadRun(void* arg)
{
    QStatus status;
    MultipleOpenHashHandleTestResult* result = reinterpret_cast<MultipleOpenHashHandleTestResult*>(arg);
    for (int algorithm = 0; algorithm < CngCache::ALGORITHM_COUNT; algorithm++) {
        status = cngCache.OpenHashHandle(static_cast<Crypto_Hash::Algorithm>(algorithm), false);
        if (ER_OK != status) {
            return reinterpret_cast<ThreadReturn>(status);
        }
        result->handles[algorithm][0] = cngCache.algHandles[algorithm][0];

        status = cngCache.OpenHashHandle(static_cast<Crypto_Hash::Algorithm>(algorithm), true);
        if (ER_OK != status) {
            return reinterpret_cast<ThreadReturn>(status);
        }
        result->handles[algorithm][1] = cngCache.algHandles[algorithm][1];
    }

    return reinterpret_cast<ThreadReturn>(ER_OK);
}

TEST(CryptoCNGMultithreadTest, MultipleOpenHashHandle)
{
    MultipleOpenHashHandleTestResult results[numThreads];

    std::vector<std::unique_ptr<Thread> > threads(numThreads);

    for (uint8_t trial = 0; trial < trials; trial++) {
        memset(results, 0, sizeof(results));

        Crypto::Shutdown();
        ASSERT_EQ(ER_OK, Crypto::Init());

        for (size_t i = 0; i < threads.size(); i++) {
            threads[i].reset(new Thread("", MultipleOpenHashHandleThreadRun, false));
        }

        for (size_t i = 0; i < threads.size(); i++) {
            ASSERT_EQ(ER_OK, threads[i]->Start(&results[i]));
        }

        for (size_t i = 0; i < threads.size(); i++) {
            ASSERT_EQ(ER_OK, threads[i]->Join());
            ASSERT_FALSE(threads[i]->IsRunning());
            ASSERT_EQ(reinterpret_cast<ThreadReturn>(ER_OK), threads[i]->GetExitValue());
        }

        for (size_t i = 1; i < ArraySize(results); i++) {
            for (int alg = 0; alg < CngCache::ALGORITHM_COUNT; alg++) {
                EXPECT_NE(results[i].handles[alg][0], results[i].handles[alg][1]);
                EXPECT_EQ(results[0].handles[alg][0], results[i].handles[alg][0]);
                EXPECT_EQ(results[0].handles[alg][1], results[i].handles[alg][1]);
            }
        }

    }

    Crypto::Shutdown();
}

static ThreadReturn STDCALL MultipleOpenAesHandleThreadRun(void* arg)
{
    QStatus status;
    BCRYPT_ALG_HANDLE* handles = reinterpret_cast<BCRYPT_ALG_HANDLE*>(arg);
    status = cngCache.OpenCcmHandle();
    if (ER_OK != status) {
        return reinterpret_cast<ThreadReturn>(status);
    }
    handles[0] = cngCache.ccmHandle;

    status = cngCache.OpenEcbHandle();
    if (ER_OK != status) {
        return reinterpret_cast<ThreadReturn>(status);
    }
    handles[1] = cngCache.ecbHandle;

    return reinterpret_cast<ThreadReturn>(ER_OK);
}

TEST(CryptoCNGMultithreadTest, MultipleOpenAesHandle)
{
    BCRYPT_ALG_HANDLE handles[numThreads][2];
    std::vector<std::unique_ptr<Thread> > threads(numThreads);

    for (uint8_t trial = 0; trial < trials; trial++) {
        memset(handles, 0, sizeof(handles));

        Crypto::Shutdown();
        ASSERT_EQ(ER_OK, Crypto::Init());

        for (size_t i = 0; i < threads.size(); i++) {
            threads[i].reset(new Thread("", MultipleOpenAesHandleThreadRun, false));
        }

        for (size_t i = 0; i < threads.size(); i++) {
            ASSERT_EQ(ER_OK, threads[i]->Start(&handles[i]));
        }

        for (size_t i = 0; i < threads.size(); i++) {
            ASSERT_EQ(ER_OK, threads[i]->Join());
            ASSERT_FALSE(threads[i]->IsRunning());
            ASSERT_EQ(reinterpret_cast<ThreadReturn>(ER_OK), threads[i]->GetExitValue());
        }

        for (size_t i = 1; i < ArraySize(handles); i++) {
            for (int alg = 0; alg < CngCache::ALGORITHM_COUNT; alg++) {
                EXPECT_NE(handles[i][0], handles[i][1]);
                EXPECT_EQ(handles[0][0], handles[i][0]);
                EXPECT_EQ(handles[0][1], handles[i][1]);
            }
        }

    }

    Crypto::Shutdown();
}

static ThreadReturn STDCALL MultipleOpenEcdsaHandleThreadRun(void* arg)
{
    QStatus status;
    BCRYPT_ALG_HANDLE* handles = reinterpret_cast<BCRYPT_ALG_HANDLE*>(arg);
    for (int algorithm = 0; algorithm < CngCache::ECDSA_ALGORITHM_COUNT; algorithm++) {
        status = cngCache.OpenEcdsaHandle(algorithm);
        if (ER_OK != status) {
            return reinterpret_cast<ThreadReturn>(status);
        }
        handles[algorithm] = cngCache.ecdsaHandles[algorithm];
    }

    return reinterpret_cast<ThreadReturn>(ER_OK);
}

TEST(CryptoCNGMultithreadTest, MultipleOpenEcdsaHandle)
{
    BCRYPT_ALG_HANDLE handles[numThreads][CngCache::ECDSA_ALGORITHM_COUNT];
    std::vector<std::unique_ptr<Thread> > threads(numThreads);

    for (uint8_t trial = 0; trial < trials; trial++) {
        memset(handles, 0, sizeof(handles));

        Crypto::Shutdown();
        ASSERT_EQ(ER_OK, Crypto::Init());

        for (size_t i = 0; i < threads.size(); i++) {
            threads[i].reset(new Thread("", MultipleOpenEcdsaHandleThreadRun, false));
        }

        for (size_t i = 0; i < threads.size(); i++) {
            ASSERT_EQ(ER_OK, threads[i]->Start(&handles[i]));
        }

        for (size_t i = 0; i < threads.size(); i++) {
            ASSERT_EQ(ER_OK, threads[i]->Join());
            ASSERT_FALSE(threads[i]->IsRunning());
            ASSERT_EQ(reinterpret_cast<ThreadReturn>(ER_OK), threads[i]->GetExitValue());
        }

        for (size_t i = 1; i < ArraySize(handles); i++) {
            for (int alg = 0; alg < CngCache::ECDSA_ALGORITHM_COUNT; alg++) {
                EXPECT_EQ(handles[0][alg], handles[i][alg]);
            }
        }

    }

    Crypto::Shutdown();
}

static ThreadReturn STDCALL MultipleOpenEcdhHandleThreadRun(void* arg)
{
    QStatus status;
    BCRYPT_ALG_HANDLE* handles = reinterpret_cast<BCRYPT_ALG_HANDLE*>(arg);
    for (int algorithm = 0; algorithm < CngCache::ECDH_ALGORITHM_COUNT; algorithm++) {
        status = cngCache.OpenEcdhHandle(algorithm);
        if (ER_OK != status) {
            return reinterpret_cast<ThreadReturn>(status);
        }
        handles[algorithm] = cngCache.ecdhHandles[algorithm];
    }

    return reinterpret_cast<ThreadReturn>(ER_OK);
}

TEST(CryptoCNGMultithreadTest, MultipleOpenEcdhHandle)
{
    BCRYPT_ALG_HANDLE handles[numThreads][CngCache::ECDH_ALGORITHM_COUNT];
    std::vector<std::unique_ptr<Thread> > threads(numThreads);

    for (uint8_t trial = 0; trial < trials; trial++) {
        memset(handles, 0, sizeof(handles));

        Crypto::Shutdown();
        ASSERT_EQ(ER_OK, Crypto::Init());

        for (size_t i = 0; i < threads.size(); i++) {
            threads[i].reset(new Thread("", MultipleOpenEcdhHandleThreadRun, false));
        }

        for (size_t i = 0; i < threads.size(); i++) {
            ASSERT_EQ(ER_OK, threads[i]->Start(&handles[i]));
        }

        for (size_t i = 0; i < threads.size(); i++) {
            ASSERT_EQ(ER_OK, threads[i]->Join());
            ASSERT_FALSE(threads[i]->IsRunning());
            ASSERT_EQ(reinterpret_cast<ThreadReturn>(ER_OK), threads[i]->GetExitValue());
        }

        for (size_t i = 1; i < ArraySize(handles); i++) {
            for (int alg = 0; alg < CngCache::ECDH_ALGORITHM_COUNT; alg++) {
                EXPECT_EQ(handles[0][alg], handles[i][alg]);
            }
        }

    }

    Crypto::Shutdown();
}


#endif /* CRYPTO_CNG */