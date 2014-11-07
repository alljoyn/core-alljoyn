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