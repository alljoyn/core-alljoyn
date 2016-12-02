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
#import "AJNHandle.h"

/**
 *  The base class for all AllJoyn API objects
 */
@interface AJNObject : NSObject<AJNHandle>

/** A handle to the C++ API object associated with this objective-c class */
@property (nonatomic, assign) AJNHandle handle;

/** Initialize the API object
    @param handle The handle to the C++ API object associated with this objective-c API object
 */
- (id)initWithHandle:(AJNHandle)handle;

/** Initialize the API object
   @param handle The handle to the C++ API object associated with this objective-c API object.
   @param deletionFlag A flag indicating whether or not the objective-c class should call delete on the handle when dealloc is called.
 */
- (id)initWithHandle:(AJNHandle)handle shouldDeleteHandleOnDealloc:(BOOL)deletionFlag;

@end