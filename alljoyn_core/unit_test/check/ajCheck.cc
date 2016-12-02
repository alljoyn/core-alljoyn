/******************************************************************************
 *
 *
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

#include <alljoyn/Init.h>
#include <alljoyn/Status.h>

#include <qcc/Debug.h>

#include <string.h>

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


/** Main entry point */
int CDECL_CALL main(int argc, char** argv, char** envArg)
{
    if (AllJoynInit() != ER_OK) {
        return 1;
    }
    if (AllJoynRouterInit() != ER_OK) {
        AllJoynShutdown();
        return 1;
    }

    int status = 0;
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    if (!IsDebugOn(envArg)) {
        QCC_RegisterOutputCallback(DebugOut, NULL);
    }

    printf("\n Running alljoyn_core unit test\n");
    testing::InitGoogleTest(&argc, argv);
    status = RUN_ALL_TESTS();

    printf("%s exiting with status %d \n", argv[0], status);

    AllJoynRouterShutdown();
    AllJoynShutdown();
    return (int) status;
}