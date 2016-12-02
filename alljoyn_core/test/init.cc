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
#include <alljoyn/Init.h>
#include <alljoyn/Status.h>

int CDECL_CALL main(int argc, char* argv[])
{
    QCC_UNUSED(argc);
    QCC_UNUSED(argv);
    if (AllJoynInit() != ER_OK) {
        return 1;
    }
#ifdef ROUTER
    if (AllJoynRouterInit() != ER_OK) {
        AllJoynShutdown();
        return 1;
    }
    AllJoynRouterShutdown();
#endif
    AllJoynShutdown();
    return 0;
}