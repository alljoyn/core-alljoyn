////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013-2014, AllSeen Alliance. All rights reserved.
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

#import <alljoyn/Session.h>
#import "AJNSessionOptions.h"

/** Invalid SessionPort value used to indicate that BindSessionPort should choose any available port */
const AJNSessionPort kAJNSessionPortAny = 0;

/** Invalid SessionId value used to indicate that a signal should be emitted on all sessions hosted by this bus attachment */
const AJNSessionId kAJNSessionIdAllHosted = 0xffffffff;

/** Proximity types */
const AJNProximity kAJNProximityAny      = 0xFF;
const AJNProximity kAJNProximityPhysical = 0x01;
const AJNProximity kAJNProximityNetwork  = 0x02;

@interface AJNObject(Private)

/**
 * Flag indicating whether or not the object pointed to by handle should be deleted when an instance of this class is deallocated.
 */
@property (nonatomic) BOOL shouldDeleteHandleOnDealloc;

@end


@implementation AJNSessionOptions

/**
 * Helper to return the C++ API object that is encapsulated by this objective-c class
 */
- (ajn::SessionOpts*)sessionOpts
{
    return (ajn::SessionOpts*)self.handle;
}

- (AJNTrafficType)trafficType
{
    return (AJNTrafficType)self.sessionOpts->traffic;
}

- (void)setTrafficType:(AJNTrafficType)trafficType
{
    self.sessionOpts->traffic = (ajn::SessionOpts::TrafficType)trafficType;
}

- (BOOL)isMultipoint
{
    return self.sessionOpts->isMultipoint;
}

- (void)setIsMultipoint:(BOOL)isMultipoint
{
    self.sessionOpts->isMultipoint = isMultipoint;
}

- (AJNProximity)proximity
{
    return self.sessionOpts->proximity;
}

- (void)setProximity:(AJNProximity)proximity
{
    self.sessionOpts->proximity = proximity;
}

- (AJNTransportMask)transports
{
    return self.sessionOpts->transports;
}

- (void)setTransports:(AJNTransportMask)transports
{
    self.sessionOpts->transports = transports;
}

- (id)initWithTrafficType:(AJNTrafficType)traffic supportsMultipoint:(BOOL)isMultipoint proximity:(AJNProximity)proximity transportMask:(AJNTransportMask)transports
{
    self = [super init];
    if (self) {
        ajn::SessionOpts *options = new ajn::SessionOpts((ajn::SessionOpts::TrafficType)traffic, isMultipoint == YES, proximity, transports);
        self.handle = options;
        self.shouldDeleteHandleOnDealloc = YES;
    }
    return self;
}

- (id)init
{
    self = [super init];
    if (self) {
        ajn::SessionOpts *options = new ajn::SessionOpts();
        self.handle = options;
        self.shouldDeleteHandleOnDealloc = YES;
    }
    return self;
}

- (void)dealloc
{
    if (self.shouldDeleteHandleOnDealloc) {
        delete self.sessionOpts;
        self.handle = nil;
    }
}

- (BOOL)isCompatibleWithSessionOptions:(AJNSessionOptions*)sessionOptions
{
    return self.sessionOpts->IsCompatible(*sessionOptions.sessionOpts);
}

- (BOOL)isEqual:(id)object
{
    BOOL result = NO;
    if ([object isKindOfClass:[AJNSessionOptions class]]) {
        result = *self.sessionOpts == *[object sessionOpts];
    }
    return result;
}

- (BOOL)isLessThanSessionOptions:(AJNSessionOptions*)sessionOptions
{
    return *self.sessionOpts < *[sessionOptions sessionOpts];
}

@end
