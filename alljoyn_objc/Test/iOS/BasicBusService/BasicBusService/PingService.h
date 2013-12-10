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

#import <Foundation/Foundation.h>
#import "AJNSessionOptions.h"

////////////////////////////////////////////////////////////////////////////////
//
// A simple delegate for the PingService
//
@protocol PingServiceDelegate <NSObject>

// transport mask to use for network communication
//
@property (nonatomic, readonly) AJNTransportMask transportType;

// The delegate is called once a client joins a session with the service
//
- (void)client:(NSString *)clientName didJoinSession:(AJNSessionId)sessionId;

// The delegate is called when a client leaves a session
//
- (void)client:(NSString *)clientName didLeaveSession:(AJNSessionId)sessionId;

// Send updates on internal state of Ping Service
//
- (void)receivedStatusMessage:(NSString*)message;

@end
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// A simple AllJoyn service that exposes a ping object
//
@interface PingService : NSObject

@property (nonatomic, weak) id<PingServiceDelegate> delegate;

- (id)initWithDelegate:(id<PingServiceDelegate>)delegate;

- (void)start:(NSString *)serviceName;
- (void)stop;

+ (PingService *)sharedInstance;

@end
////////////////////////////////////////////////////////////////////////////////
