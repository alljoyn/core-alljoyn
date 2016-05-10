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
#include <gtest/gtest.h>

#include <alljoyn/Init.h>

#include <qcc/Debug.h>

#include <string.h>
#include <climits>

/**
 * Needed to allow automatic memory dumps for Windows Jenkins builds.
 * For C++ exception the default exception filter will cause a UI
 * prompt to appear instead of running the default debugger.
 */
#if defined(QCC_OS_GROUP_WINDOWS) && defined(QCC_CRASH_DUMP_SUPPORT)
static LONG DummyExceptionFilter(LPEXCEPTION_POINTERS pointers) {
    QCC_UNUSED(pointers);
    return EXCEPTION_CONTINUE_SEARCH;
}

static void SetExceptionHandling() {
    SetUnhandledExceptionFilter(DummyExceptionFilter);
}
#else
static void SetExceptionHandling() {
    return;
}
#endif

static void DebugOut(DbgMsgType type, const char* module, const char* msg, void* context)
{
    QCC_UNUSED(type);
    QCC_UNUSED(module);
    QCC_UNUSED(msg);
    QCC_UNUSED(context);
    // Do nothing to suppress AJ errors and debug prints.
}

static bool IsDebugOn(char** env)
{
    while (env && *env) {
        if (strncmp(*env, "ER_DEBUG_", 9) == 0) {
            return true;
        }
        ++env;
    }
    return false;
}

static void Usage(void)
{
    printf("Usage: ajtest [-h] [--timeout_multiplier <value>] [gtest options]\n\n");
    printf("Options:\n");
    printf("   -h                           = Print this help message\n");
    printf("   --timeout_multiplier <value> = Various timeouts multiplier, expects positive integer value in range [1, 100] and defaults to 1. Useful for example when running ajtest under Valgrind.\n");
    printf("\n");
}

/*
 * Multiplier factor allowing to scale hardcoded UT timeouts for calls like:
 * - Condition::TimedWait
 * - qcc::Sleep
 * - Event::Wait
 * - ProxyBusObject::MethodCall
 * - RemoteEndpoint::Join
 */
uint32_t s_globalTimerMultiplier = 1;

/** Main entry point */
int CDECL_CALL main(int argc, char** argv, char** envArg)
{
    SetExceptionHandling();

    if (AllJoynInit() != ER_OK) {
        return 1;
    }
#ifdef ROUTER
    if (AllJoynRouterInit() != ER_OK) {
        AllJoynShutdown();
        return 1;
    }
#endif

    int status = 0;
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    if (!IsDebugOn(envArg)) {
        QCC_RegisterOutputCallback(DebugOut, NULL);
    }

    for (int i = 1; i < argc; ++i) {
        if (0 == strcmp("--timeout_multiplier", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option --timeout_multiplier requires a parameter\n");
                Usage();
                exit(1);
            } else {
                unsigned long int gtm = strtoul(argv[i], NULL, 10);
                if ((gtm > 0) && (gtm <= 100)) {
                    s_globalTimerMultiplier = gtm;
                } else {
                    printf("out of range --timeout_multiplier value\n");
                    Usage();
                    exit(1);
                }
            }
        } else if (0 == strcmp("-h", argv[i])) {
            Usage();
            exit(0);
        }
    }

    printf("\n Running alljoyn_core unit test\n");
    testing::InitGoogleTest(&argc, argv);
    status = RUN_ALL_TESTS();

    printf("%s exiting with status %d \n", argv[0], status);

#ifdef ROUTER
    AllJoynRouterShutdown();
#endif
    AllJoynShutdown();
    return (int) status;
}
