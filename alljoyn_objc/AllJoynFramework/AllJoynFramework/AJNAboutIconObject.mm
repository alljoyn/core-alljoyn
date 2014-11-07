////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014, AllSeen Alliance. All rights reserved.
//
//    Permission to use, copy, modify, and/or distribute this software for any
//    purpose with or without fee is hereby granted, provided that the above
//    copyright notice and this permission notice appear in all copies.
//
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
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