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

#import "AJNObject.h"

@interface AJNObject()

/**
 * Flag indicating whether or not the object pointed to by handle should be deleted when an instance of this class is deallocated.
 */
@property (nonatomic) BOOL shouldDeleteHandleOnDealloc;

@end

@implementation AJNObject

@synthesize handle = _handle;
@synthesize shouldDeleteHandleOnDealloc = _shouldDeleteHandleOnDealloc;

- (id)initWithHandle:(AJNHandle)handle
{
    self = [super init];
    if (self) {
        self.handle = handle;
        self.shouldDeleteHandleOnDealloc = NO;
    }
    return self;
}

- (id)initWithHandle:(AJNHandle)handle shouldDeleteHandleOnDealloc:(BOOL)deletionFlag
{
    self = [super init];
    if (self) {
        self.handle = handle;
        self.shouldDeleteHandleOnDealloc = deletionFlag;
    }
    return self;
}

@end