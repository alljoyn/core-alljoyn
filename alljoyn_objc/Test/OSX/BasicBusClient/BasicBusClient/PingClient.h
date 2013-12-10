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

#import "AJNBusAttachment.h"

// A simple delegate for the PingClient
//
@protocol PingClientDelegate <NSObject>

@property (nonatomic, readonly) AJNTransportMask transportType;

// The delegate is called once a connection is established between the client
// and the service
//
- (void)didConnectWithService:(NSString *)serviceName;

// The delegate is called when a service session is lost
//
- (void)shouldDisconnectFromService:(NSString *)serviceName;

// Send updates on internal state of Ping Client
//
- (void)receivedStatusMessage:(NSString*)message;

@end

// A simple AllJoyn client
//
@interface PingClient : NSObject

@property (nonatomic, strong) id<PingClientDelegate> delegate;

- (id)initWithDelegate:(id<PingClientDelegate>)delegate;

- (void)connectToService:(NSString *)serviceName;
- (void)disconnect;

- (NSString *)sendPingToService:(NSString *)str1;

+ (PingClient *)sharedInstance;

@end
