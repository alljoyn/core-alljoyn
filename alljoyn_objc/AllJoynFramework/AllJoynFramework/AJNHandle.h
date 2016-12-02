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

#import <Foundation/Foundation.h>

/**
 * Type definition for the AJNHandle, which is used to refer to C++ API objects.
 */
typedef void* AJNHandle;

/**
 * The objective-c base class that serves as a wrapper for a C++ API class. The instance
 * of the C++ API object is maintained in the handle property.
 */
@protocol AJNHandle <NSObject>

/**
 * Holds a pointer to a C++ API object.
 */
@property (nonatomic, assign) AJNHandle handle;

@end