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

#include <gtest/gtest.h>

#include <alljoyn/Status.h>

#include <qcc/StaticGlobals.h>

/**
 * Needed to allow automatic memory dumps for Windows Jenkins builds.
 * For C++ exception the default exception filter will cause a UI
 * prompt to appear instead of running the default debugger.
 */
#if defined(QCC_OS_GROUP_WINDOWS) && defined(ALLJOYN_CRASH_DUMP_SUPPORT)
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

/** Main entry point */
int CDECL_CALL main(int argc, char**argv, char**)
{
    SetExceptionHandling();

    if (qcc::Init() != ER_OK) {
        return -1;
    }

    int status = 0;
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    printf("\n Running common unit test \n");
    testing::InitGoogleTest(&argc, argv);
    status = RUN_ALL_TESTS();

    printf("%s exiting with status %d \n", argv[0], status);

    qcc::Shutdown();
    return (int) status;
}