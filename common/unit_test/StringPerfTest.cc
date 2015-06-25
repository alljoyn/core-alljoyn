/******************************************************************************
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
#include <gtest/gtest.h>
#include <qcc/String.h>


#include <ctype.h>
#include <iostream>
#include <set>
#include <string>
#include <unordered_set>

#include <qcc/String.h>
#include <qcc/Util.h>

#define ITERATIONS 1000000
#define MAX_TEST_DATA 1024

#if defined(QCC_OS_GROUP_POSIX)
#include <time.h>
#if defined(QCC_OS_DARWIN)
#include <sys/time.h>
#include <mach/mach_time.h>
#include <mach/clock.h>
#include <mach/mach.h>
#endif

static void platform_gettime(struct timespec* ts, bool useMonotonic)
{
#if defined(QCC_OS_DARWIN)
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    ts->tv_sec = mts.tv_sec;
    ts->tv_nsec = mts.tv_nsec;

#else
    clock_gettime(useMonotonic ? CLOCK_MONOTONIC : CLOCK_REALTIME, ts);
#endif
}
static uint64_t usTime()
{
    struct timespec ts;
    uint64_t ret_val;

    platform_gettime(&ts, true);

    ret_val = ((uint64_t)(ts.tv_sec)) * 1000000;
    ret_val += (uint64_t)ts.tv_nsec / 1000;

    return ret_val;
}


#elif defined(QCC_OS_GROUP_WINDOWS)
static LARGE_INTEGER epoch;
static LARGE_INTEGER freq;
uint64_t usTime()
{
    LARGE_INTEGER nowAbs;
    QueryPerformanceCounter(&nowAbs);
    auto now = nowAbs.QuadPart - epoch.QuadPart;

    now *= 1000000;
    now /= freq.QuadPart;
    return (uint64_t)now;
}
#endif

static const char rsrc[] = "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ`~!@#$%^&*()-_=+[{]}|;:,<.>/?";

template <class StringType, class HashFunction>
class RunTest {
    const char* testData;
    const size_t size;
  public:
    RunTest(const char* testData, size_t size) : testData(testData), size(size) { }
    size_t operator()()
    {
        size_t ret = 0;
        std::set<StringType> sortedSet;
        std::unordered_set<StringType, HashFunction> hashSet;

        HashFunction HashFn;

        StringType strA(testData, size);
        StringType strB(testData, size);
        StringType strC(testData, size);
        strA[0] = 'A';
        strA[strA.size() - 1] = 'A';
        strB[strB.size() - 1] = 'B';
        strC[0] = 'C';

        StringType strNeedleFirst(strA.c_str(), 8);
        StringType strNeedleLast(strA.c_str() + (strA.size() - 8), 8);
        StringType strNeedleNotFound(testData, 8);
        strNeedleNotFound[strNeedleNotFound.size() - 1] = '\a';  // character guaranteed to not be in strA.

        uint64_t start;
        uint64_t stop;

        start = usTime();
        for (auto i = 0; i < ITERATIONS; ++i) {
            char r = rsrc[i % (sizeof(rsrc) - 1)];  // force compiler to not optimize comparison out to outside of loop
            strA[2] = r;
            strC[2] = r;

            ret += (strA != strC) ? 1 : 0;
        }
        stop = usTime();
        std::cout << "    String comparison - first character different (ret=" << ret << "): " << (stop - start) / 1000.0 << std::endl;

        start = usTime();
        for (auto i = 0; i < ITERATIONS; ++i) {
            char r = rsrc[i % (sizeof(rsrc) - 1)];  // force compiler to not optimize comparison out to outside of loop
            strA[2] = r;
            strB[2] = r;

            ret += (strA != strB) ? 1 : 0;
        }
        stop = usTime();
        std::cout << "    String comparison - last character different (ret=" << ret << "): " << (stop - start) / 1000.0 << std::endl;

        start = usTime();
        for (auto i = 0; i < ITERATIONS; ++i) {
            char r = rsrc[i % (sizeof(rsrc) - 1)];  // force compiler to not optimize comparison out to outside of loop
            strA[2] = r;
            strC[2] = r;

            ret += (strA < strC) ? 1 : 0;
        }
        stop = usTime();
        std::cout << "    String less than - first character (ret=" << ret << "): " << (stop - start) / 1000.0 << std::endl;

        start = usTime();
        for (auto i = 0; i < ITERATIONS; ++i) {
            char r = rsrc[i % (sizeof(rsrc) - 1)];  // force compiler to not optimize comparison out to outside of loop
            strA[2] = r;
            strB[2] = r;

            ret += (strA < strB) ? 1 : 0;
        }
        stop = usTime();
        std::cout << "    String less than - last character (ret=" << ret << "): " << (stop - start) / 1000.0 << std::endl;

        start = usTime();
        for (auto i = 0; i < ITERATIONS; ++i) {
            char r = rsrc[i % (sizeof(rsrc) - 1)];  // force compiler to not optimize comparison out to outside of loop
            strA[2] = r;
            strNeedleFirst[2] = r;

            ret += (strA.find(strNeedleFirst) == 0) ? 1 : 0;
        }
        stop = usTime();
        std::cout << "    String find - match beginning (ret=" << ret << "): " << (stop - start) / 1000.0 << std::endl;

        start = usTime();
        for (auto i = 0; i < ITERATIONS; ++i) {
            char r = rsrc[i % (sizeof(rsrc) - 1)];  // force compiler to not optimize comparison out to outside of loop
            strA[strA.size() - 4] = r;
            strNeedleLast[strNeedleLast.size() - 4] = r;

            ret += (strA.find(strNeedleLast) == (strA.size() - strNeedleLast.size())) ? 1 : 0;
        }
        stop = usTime();
        std::cout << "    String find - match end (ret=" << ret << "): " << (stop - start) / 1000.0 << std::endl;

        start = usTime();
        for (auto i = 0; i < ITERATIONS; ++i) {
            char r = rsrc[i % (sizeof(rsrc) - 1)];  // force compiler to not optimize comparison out to outside of loop
            strA[2] = r;
            strNeedleNotFound[2] = r;

            ret += (strA.find(strNeedleNotFound) == StringType::npos) ? 1 : 0;
        }
        stop = usTime();
        std::cout << "    String find - match not found (ret=" << ret << "): " << (stop - start) / 1000.0 << std::endl;

        start = usTime();
        for (auto i = 0; i < ITERATIONS; ++i) {
            char r = rsrc[i % (sizeof(rsrc) - 1)];  // force compiler to not optimize comparison out to outside of loop
            strA[(i / (sizeof(rsrc) - 1)) % strA.size()] = r;
            StringType dest = strA.c_str();  // force a strcpy()
            ret += (strA[2] == dest[2]) ? 1 : 0;
        }
        stop = usTime();
        std::cout << "    String copy (ret=" << ret << "): " << (stop - start) / 1000.0 << std::endl;

        strA = StringType(testData, size); // reset strA
        size_t hash = 0;
        start = usTime();
        for (auto i = 0; i < ITERATIONS; ++i) {
            char r = rsrc[i % (sizeof(rsrc) - 1)];  // force compiler to not optimize comparison out to outside of loop
            strA[(i / (sizeof(rsrc) - 1)) % strA.size()] = r;
            hash ^= HashFn(strA);
            ret += (hash != 0) ? 1 : 0;
        }
        stop = usTime();
        std::cout << "    Hash string (ret=" << ret << "   hash=" << std::hex << hash << std::dec << "): " << (stop - start) / 1000.0 << std::endl;

        strB = StringType(testData, size); // reset strB
        start = usTime();
        for (auto i = 0; i < ITERATIONS; ++i) {
            char r = rsrc[i % (sizeof(rsrc) - 1)];  // force compiler to not optimize comparison out to outside of loop
            strB[(i / (sizeof(rsrc) - 1)) % strB.size()] = r;
            auto x = hashSet.insert(strB);
            ret += x.second ? 1 : 0;
        }
        stop = usTime();
        std::cout << "    hash set insert (ret=" << ret << "  set size: " << hashSet.size() << "): " << (stop - start) / 1000.0 << std::endl;

        strC = StringType(testData, size); // reset strC
        start = usTime();
        for (auto i = 0; i < ITERATIONS; ++i) {
            char r = rsrc[i % (sizeof(rsrc) - 1)];  // force compiler to not optimize comparison out to outside of loop
            strC[(i / (sizeof(rsrc) - 1)) % strC.size()] = r;
            ret += (hashSet.find(strC) != hashSet.end()) ? 1 : 0;
        }
        stop = usTime();
        std::cout << "    hash set find (ret=" << ret << "): " << (stop - start) / 1000.0 << std::endl;

        strA = StringType(testData, size); // reset strA
        start = usTime();
        for (auto i = 0; i < ITERATIONS; ++i) {
            char r = rsrc[i % (sizeof(rsrc) - 1)];  // force compiler to not optimize comparison out to outside of loop
            strA[(i / (sizeof(rsrc) - 1)) % strA.size()] = r;
            auto x = sortedSet.insert(strA);
            ret += x.second ? 1 : 0;
        }
        stop = usTime();
        std::cout << "    Sorted set insert (ret=" << ret << "  set size: " << sortedSet.size() << "): " << (stop - start) / 1000.0 << std::endl;

        strA = StringType(testData, size); // reset strA
        start = usTime();
        for (auto i = 0; i < ITERATIONS; ++i) {
            char r = rsrc[i % (sizeof(rsrc) - 1)];  // force compiler to not optimize comparison out to outside of loop
            strA[(i / (sizeof(rsrc) - 1)) % strA.size()] = r;
            ret += (sortedSet.find(strA) != sortedSet.end()) ? 1 : 0;
        }
        stop = usTime();
        std::cout << "    Sorted set find (ret=" << ret << "): " << (stop - start) / 1000.0 << std::endl;

        return ret;
    }

  private:
    RunTest(const RunTest& other);
    RunTest& operator=(const RunTest& other);
};


namespace std {
template <>
struct hash<qcc::String> {
    inline size_t operator()(const qcc::String& k) const { return qcc::hash_string(k.c_str()); }
};
}


void RunTestWrapper(const char* testData, size_t size)
{
    RunTest<std::string, std::hash<std::string> > stdStr(testData, size);
    RunTest<qcc::String, std::hash<qcc::String> > qccStr(testData, size);
    std::cout << "std::string - length " << size << ":" << std::endl;
    stdStr();
    std::cout << "qcc::String - length " << size << ":" << std::endl;
    qccStr();
}

TEST(StringPerfTest, StringPerfTest)
{
#if defined(QCC_OS_GROUP_WINDOWS)
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&epoch);
#endif

    char* testData = new char[MAX_TEST_DATA + 1];
    for (size_t i = 0; i < sizeof(testData) - 1; ++i) {
        testData[i] = rsrc[i % (sizeof(rsrc) - 1)];
    }
    testData[MAX_TEST_DATA] = '\0';

    std::cout << "----------------------------------------------------" << std::endl;
    RunTestWrapper(testData, 16);
    std::cout << "----------------------------------------------------" << std::endl;
    RunTestWrapper(testData, 64);
    std::cout << "----------------------------------------------------" << std::endl;
    RunTestWrapper(testData, 256);
    std::cout << "----------------------------------------------------" << std::endl;
    RunTestWrapper(testData, MAX_TEST_DATA);
    std::cout << "----------------------------------------------------" << std::endl;

    delete[] testData;
}
