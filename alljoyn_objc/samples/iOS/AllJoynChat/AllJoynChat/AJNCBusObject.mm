////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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

#import "AJNCBusObject.h"
#import "AJNCConversation.h"
#import "AJNCConstants.h"
#import "AJNBusAttachment.h"
#import "AJNCBusObjectImpl.h"
#import "AJNCMessage.h"
#import "AJNInterfaceDescription.h"

@interface AJNCBusObject()

@property (nonatomic) NSInteger sessionType;

@end

@implementation AJNCBusObject

@synthesize session = _session;
@synthesize sessionType = _sessionType;
@synthesize delegate = _delegate;

- (id)initWithBusAttachment:(AJNBusAttachment *)busAttachment onServicePath:(NSString *)path
{
    self = [super initWithBusAttachment:busAttachment onPath:path];
    if (self) {
        // create the internal C++ bus object
        //
        AJNCBusObjectImpl *busObject = new AJNCBusObjectImpl(*((ajn::BusAttachment*)busAttachment.handle), [path UTF8String], self);
        
        self.handle = busObject;
    }
    
    return self;
}

- (void)sendChatMessage:(NSString*)message onSession:(AJNSessionId)sessionId
{
    QStatus status = static_cast<AJNCBusObjectImpl*>(self.handle)->SendChatSignal([message UTF8String], sessionId);
    if (status != ER_OK) {
        NSLog(@"ERROR: sendChatMessage failed. %@", [AJNStatus descriptionForStatusCode:status]);
    }
}

- (void)chatMessageReceived:(NSString *)message from:(NSString*)sender onObjectPath:(NSString *)path forSession:(AJNSessionId)sessionId
{
    if (self.session.identifier != sessionId) {
        return;
    }
    
    NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
    [formatter setTimeStyle:NSDateFormatterMediumStyle];
    [formatter setDateStyle:NSDateFormatterShortStyle];
    
    AJNCMessage *chatMessage = [[AJNCMessage alloc] initWithText:message fromSender:sender atDateTime:[formatter stringFromDate:[NSDate date]]];
    [self.session.messages addObject:chatMessage];

    [self.delegate chatMessageReceived:message from:sender onObjectPath:path forSession:sessionId];
}

@end
