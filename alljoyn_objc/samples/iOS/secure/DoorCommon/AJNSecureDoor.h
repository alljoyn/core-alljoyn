////////////////////////////////////////////////////////////////////////////////
//    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
//    Project (AJOSP) Contributors and others.
//    
//    SPDX-License-Identifier: Apache-2.0
//    
//    All rights reserved. This program and the accompanying materials are
//    made available under the terms of the Apache License, Version 2.0
//    which accompanies this distribution, and is available at
//    http://www.apache.org/licenses/LICENSE-2.0
//    
//    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
//    Alliance. All rights reserved.
//    
//    Permission to use, copy, modify, and/or distribute this software for
//    any purpose with or without fee is hereby granted, provided that the
//    above copyright notice and this permission notice appear in all
//    copies.
//    
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
//    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
//    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
//    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
//    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
//    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
//    PERFORMANCE OF THIS SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  ALLJOYN MODELING TOOL - GENERATED CODE
//
////////////////////////////////////////////////////////////////////////////////
//
//  DO NOT EDIT
//
//  Add a category or subclass in separate .h/.m files to extend these classes
//
////////////////////////////////////////////////////////////////////////////////
//
//  AJNSecureDoor.h
//
////////////////////////////////////////////////////////////////////////////////

#import <Foundation/Foundation.h>
#import "AJNBusAttachment.h"
#import "AJNBusInterface.h"
#import "AJNProxyBusObject.h"


////////////////////////////////////////////////////////////////////////////////
//
// DoorDelegate Bus Interface
//
////////////////////////////////////////////////////////////////////////////////

@protocol DoorDelegate <AJNBusInterface>


// properties
//
@property (nonatomic, readonly) BOOL State;

// methods
//
- (QStatus) open:(BOOL *)success message:(AJNMessage *)methodCallMessage;
- (QStatus) close:(BOOL *)success message:(AJNMessage *)methodCallMessage;
- (QStatus) getStateMethod:(BOOL *)state message:(AJNMessage *)methodCallMessage;

// signals
//
- (QStatus)sendstate:(BOOL )State inSession:(AJNSessionId)sessionId toDestination:(NSString *)destinationPath;

@end

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//
// DoorDelegate Signal Handler Protocol
//
////////////////////////////////////////////////////////////////////////////////

@protocol DoorDelegateSignalHandler <AJNSignalHandler>

// signals
//
- (void)didReceivestate:(BOOL )State inSession:(AJNSessionId)sessionId message:(AJNMessage *)signalMessage;

@end

@interface AJNBusAttachment(DoorDelegate)

- (void)registerDoorDelegateSignalHandler:(id<DoorDelegateSignalHandler>)signalHandler;

@end

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//
//  AJNDoor Bus Object superclass
//
////////////////////////////////////////////////////////////////////////////////

@interface AJNDoor : AJNBusObject<DoorDelegate>
{
@protected
    BOOL _State;

}
// properties
//
@property (nonatomic, readonly) BOOL State;

// methods
//
- (QStatus) open:(BOOL *)success message:(AJNMessage *)methodCallMessage;
- (QStatus) close:(BOOL *)success message:(AJNMessage *)methodCallMessage;
- (QStatus) getStateMethod:(BOOL *)state message:(AJNMessage *)methodCallMessage;

// signals
//
- (QStatus)sendstate:(BOOL )State inSession:(AJNSessionId)sessionId toDestination:(NSString *)destinationPath;

@end

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//
//  Door Proxy
//
////////////////////////////////////////////////////////////////////////////////

@interface DoorProxy : AJNProxyBusObject

// properties
//
- (QStatus)getState:(BOOL *)prop;


// methods
//
- (QStatus) open:(BOOL *)success;
- (QStatus) open:(BOOL *)success replyMessage:(AJNMessage **) replyMessage;

- (QStatus) close:(BOOL *)success;
- (QStatus) close:(BOOL *)success replyMessage:(AJNMessage **) replyMessage;

- (QStatus) getStateMethod:(BOOL *)state;
- (QStatus) getStateMethod:(BOOL *)state replyMessage:(AJNMessage **) replyMessage;


@end

////////////////////////////////////////////////////////////////////////////////
