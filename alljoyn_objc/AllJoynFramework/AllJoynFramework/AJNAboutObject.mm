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
#import <alljoyn/AboutObj.h>
#import <alljoyn/BusObject.h>
#import "AJNBusAttachment.h"
#import "AJNSessionOptions.h"
#import "AJNAboutObject.h"
#import "AJNBusObject.h"
#import "AJNStatus.h"
#import "AJNAboutDataListenerImpl.h"
#import "AJNMessageArgument.h"

using namespace ajn;

@interface AJNBusAttachment(Private)

@property (nonatomic, readonly) BusAttachment *busAttachment;

@end

@interface AJNObject(Private)

@property (nonatomic) BOOL shouldDeleteHandleOnDealloc;

@end

@interface AJNAboutObject()

/**
 * The bus attachment this object is associated with.
 */
@property (nonatomic, strong) AJNBusAttachment *bus;
@property (nonatomic, readonly) AboutObj *aboutObject;
@end


@implementation AJNAboutObject

@synthesize bus = _bus;

/**
 * Helper to return the C++ API object that is encapsulated by this objective-c class
 */
- (AboutObj*)aboutObject
{
    return static_cast<AboutObj*>(self.handle);
}

- (id)initWithBusAttachment:(AJNBusAttachment *)busAttachment withAnnounceFlag:(AJNAnnounceFlag)announceFlag
{
    self = [super init];
    if (self) {
        self.bus = busAttachment;
        self.handle = new AboutObj(*((BusAttachment*)busAttachment.handle), (BusObject::AnnounceFlag)announceFlag);
        self.shouldDeleteHandleOnDealloc = YES;
    }
    return self;
}

- (void)dealloc
{
    if (self.shouldDeleteHandleOnDealloc) {
        delete self.aboutObject;
        self.handle = nil;
    }
}

- (QStatus)announceForSessionPort:(AJNSessionPort)sessionPort withAboutDataListener:(id<AJNAboutDataListener>)aboutDataListener
{
    AJNAboutDataListenerImpl *listenerImpl = new AJNAboutDataListenerImpl(aboutDataListener);
    return self.aboutObject->Announce(sessionPort, *listenerImpl);
}

-(QStatus)unannounce
{
    return self.aboutObject->Unannounce();
}



@end