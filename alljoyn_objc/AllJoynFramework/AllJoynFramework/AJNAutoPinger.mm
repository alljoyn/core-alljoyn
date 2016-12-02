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

#import <alljoyn/BusAttachment.h>
#import <alljoyn/AutoPinger.h>
#import "AJNAutoPinger.h"
#import "AJNPingListenerImpl.h"


@interface AJNBusAttachment(Private)

@property (nonatomic, readonly) ajn::BusAttachment *busAttachment;

@end

@interface AJNObject(Private)

@property (nonatomic) BOOL shouldDeleteHandleOnDealloc;

@end

@interface AJNAutoPinger()

@property (nonatomic, strong) NSMutableArray *pingListeners;

@end

@implementation AJNAutoPinger

@synthesize pingListeners = _pingListeners;

- (id)initWithBusAttachment:(AJNBusAttachment *)busAttachment
{
    self = [super init];
    if (self) {
        self.handle = new ajn::AutoPinger(*busAttachment.busAttachment);
        self.shouldDeleteHandleOnDealloc = YES;
    }
    
    return self;
}

- (ajn::AutoPinger*)autoPinger
{
    return static_cast<ajn::AutoPinger*>(self.handle);
}

- (void)dealloc
{
    @synchronized(self.pingListeners) {
        for (NSValue *ptrValue in self.pingListeners) {
            AJNPingListenerImpl * listenerImpl = (AJNPingListenerImpl*)[ptrValue pointerValue];
            delete listenerImpl;
        }
        [self.pingListeners removeAllObjects];
    }
    
    if (self.shouldDeleteHandleOnDealloc) {
        ajn::AutoPinger *pArg = static_cast<ajn::AutoPinger*>(self.handle);
        delete pArg;
        self.handle = nil;
    }
    
}

- (void)pause
{
    self.autoPinger->Pause();
}

- (void)resume
{
    self.autoPinger->Resume();
}

- (void)addPingGroup:(NSString *)group withDelegate:(id<AJNPingListener>)listener withInterval:(uint32_t)pingInterval
{
    AJNPingListenerImpl *listenerImpl = new AJNPingListenerImpl(listener);
    @synchronized(self.pingListeners) {
        [self.pingListeners addObject:[NSValue valueWithPointer:listenerImpl]];
        self.autoPinger->AddPingGroup([group UTF8String], *listenerImpl, pingInterval);
    }
}

- (void)removePingGroup:(NSString *)group
{
    self.autoPinger->RemovePingGroup([group UTF8String]);
}

- (QStatus)setPingInterval:(NSString *)group withInterval:(uint32_t)pingInterval
{
    return self.autoPinger->SetPingInterval([group UTF8String], pingInterval);
}

- (QStatus)addDestination:(NSString *)group forDestination:(NSString *)destination
{
    return self.autoPinger->AddDestination([group UTF8String], [destination UTF8String]);
}

- (QStatus)removeDestination:(NSString *)group forDestination:(NSString *)destination shouldRemoveAll:(BOOL)removeAll
{
    return self.autoPinger->RemoveDestination([group UTF8String], [destination UTF8String], removeAll ?  true : false);
}

@end