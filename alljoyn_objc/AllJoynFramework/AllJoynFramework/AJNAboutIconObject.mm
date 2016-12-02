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
#import <alljoyn/AboutIconObj.h>
#import "AJNAboutIconObject.h"
#import "AJNAboutIcon.h"

using namespace ajn;

@interface AJNBusAttachment(Private)

@property (nonatomic, readonly) BusAttachment *busAttachment;

@end

@interface AJNAboutIconObject()

/**
 * The bus attachment this object is associated with.
 */
@property (nonatomic, strong) AJNBusAttachment *bus;
@property (nonatomic, readonly) AboutIconObj *aboutIconObject;
@property (nonatomic, strong) AJNAboutIcon *aboutIcon;
@end

@implementation AJNAboutIconObject

static uint16_t version;

@synthesize bus = _bus;
@synthesize aboutIconObject = _aboutIconObject;

+ (uint16_t)version
{
    return version;
}

/**
 * Helper to return the C++ API object that is encapsulated by this objective-c class
 */
- (AboutIconObj*)aboutIconObject
{
    return static_cast<AboutIconObj*>(self.handle);
}


- (id)initWithBusAttachment:(AJNBusAttachment *)busAttachment aboutIcon:(AJNAboutIcon *)aboutIcon
{
    self = [super init];
    if (self) {
        self.bus = busAttachment;
        self.aboutIcon = aboutIcon;
        self.handle = new AboutIconObj(*((BusAttachment*)busAttachment.handle), *(AboutIcon*)aboutIcon.handle);
        version = self.aboutIconObject->VERSION;
    }
    return self;
    
}

@end