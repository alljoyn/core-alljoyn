////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013, AllSeen Alliance. All rights reserved.
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

////////////////////////////////////////////////////////////////////////////////
//
//  AJNBusControllerDelegate protocol
//
@protocol AJNBusControllerDelegate <NSObject>

@property (nonatomic, readonly) NSString *applicationName;
@property (nonatomic, readonly) NSString *serviceName;
@property (nonatomic, readonly) AJNBusNameFlag serviceNameFlags;
@property (nonatomic, readonly) AJNSessionPort sessionPort;

@optional

- (void)didStartBus:(AJNBusAttachment *)bus;
- (void)didConnectBus:(AJNBusAttachment *)bus;

- (void)listenerDidRegisterWithBus:(AJNBusAttachment *)busAttachment;
- (void)listenerDidUnregisterWithBus:(AJNBusAttachment *)busAttachment;
- (void)didFindAdvertisedName:(NSString *)name withTransportMask:(AJNTransportMask) transport namePrefix:(NSString *)namePrefix;
- (void)didLoseAdvertisedName:(NSString *)name withTransportMask:(AJNTransportMask) transport namePrefix:(NSString *)namePrefix;
- (void)nameOwnerChanged:(NSString *)name to:(NSString *)newOwner from:(NSString *)previousOwner;
- (void)busWillStop;
- (void)busDidDisconnect;

- (void)sessionWasLost:(AJNSessionId)sessionId;
- (void)didAddMemberNamed:(NSString *)memberName toSession:(AJNSessionId)sessionId;
- (void)didRemoveMemberNamed:(NSString *)memberName fromSession:(AJNSessionId)sessionId;

- (void)didReceiveStatusMessage:(NSString *)message;

@end
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  AJNBusController protocol
//
@protocol AJNBusController <AJNBusListener, AJNSessionListener>

// Bus Attachment
//
@property (nonatomic, strong) AJNBusAttachment *bus;

// Session Options
//
@property (nonatomic) BOOL allowRemoteMessages;
@property (nonatomic) AJNTrafficType trafficType;
@property (nonatomic) AJNProximity proximityOptions;
@property (nonatomic) AJNTransportMask transportMask;
@property (nonatomic) BOOL multiPointSessionsEnabled;
@property (nonatomic, readonly) AJNSessionId sessionId;

// Connection options
//
@property (nonatomic, strong) NSString *connectionArguments;

// Initialization
//
- (id)init;
- (id)initWithBusAttachment:(AJNBusAttachment *)busAttachment;

// Bus Control
//
- (void)start;

- (void)stop;

@end
////////////////////////////////////////////////////////////////////////////////
