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
#import <alljoyn/BusAttachment.h>
#import <alljoyn/AboutIconProxy.h>
#import "AJNBusAttachment.h"
#import "AJNAboutIconProxy.h"

using namespace ajn;

@interface AJNBusAttachment(Private)
@property (nonatomic, readonly) BusAttachment *busAttachment;
@end

@interface AJNObject(Private)
@property (nonatomic) BOOL shouldDeleteHandleOnDealloc;
@end

@interface AJNAboutIconProxy()
@property (nonatomic, strong) AJNBusAttachment *bus;
@property (nonatomic, readonly) AboutIconProxy *aboutIconProxy;
@end



@implementation AJNAboutIconProxy

@synthesize bus = _bus;

/**
 * Helper to return the C++ API object that is encapsulated by this objective-c class
 */
- (AboutIconProxy*)aboutIconProxy
{
    return static_cast<AboutIconProxy*>(self.handle);
}

- (id)initWithBusAttachment:(AJNBusAttachment *)busAttachment busName:(NSString *)busName sessionId:(AJNSessionId)sessionId
{
    self = [super init];
    if (self) {
        self.bus = busAttachment;
        self.handle = new AboutIconProxy(*((BusAttachment*)busAttachment.handle), [busName UTF8String], sessionId);
    }
    return self;
}

- (QStatus)getIcon:(AJNAboutIcon *)icon
{
    QStatus status;
    AboutIcon *aboutIcon = new AboutIcon();
    status = self.aboutIconProxy->GetIcon(*aboutIcon);
    if (status == ER_OK) {
        [icon setUrl:[NSString stringWithCString:aboutIcon->url.c_str() encoding:NSUTF8StringEncoding ]];
        [icon setContent:aboutIcon->content];
        [icon setContentSize:aboutIcon->contentSize];
        [icon setMimeType:[NSString stringWithCString:aboutIcon->mimetype.c_str() encoding:NSUTF8StringEncoding ]];
    }
    return status;
}

- (QStatus)getVersion:(uint16_t *)version
{
    return self.aboutIconProxy->GetVersion(*version);
}

- (void)dealloc
{
    if (self.shouldDeleteHandleOnDealloc) {
        AboutIconProxy *pArg = static_cast<AboutIconProxy*>(self.handle);
        delete pArg;
        self.handle = nil;
    }
}


@end