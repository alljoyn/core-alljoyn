////////////////////////////////////////////////////////////////////////////////
// // 
//    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
//    Source Project Contributors and others.
//    
//    All rights reserved. This program and the accompanying materials are
//    made available under the terms of the Apache License, Version 2.0
//    which accompanies this distribution, and is available at
//    http://www.apache.org/licenses/LICENSE-2.0

////////////////////////////////////////////////////////////////////////////////

#import <Cocoa/Cocoa.h>
#import "AJNInit.h"

int main(int argc, char *argv[])
{
    if ([AJNInit alljoynInit] != ER_OK) {
        return 1;
    }
    if ([AJNInit alljoynRouterInit] != ER_OK) {
        [AJNInit alljoynShutdown];
        return 1;
    }
    int ret = NSApplicationMain(argc, (const char **)argv);
    [AJNInit alljoynRouterShutdown];
    [AJNInit alljoynShutdown];
    return ret;
}