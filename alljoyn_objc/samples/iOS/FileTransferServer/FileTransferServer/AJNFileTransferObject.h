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
//  AJNFileTransferObject.h
//
////////////////////////////////////////////////////////////////////////////////

#import <Foundation/Foundation.h>
#import "AJNBusAttachment.h"
#import "AJNBusInterface.h"
#import "AJNProxyBusObject.h"


////////////////////////////////////////////////////////////////////////////////
//
// FileTransferDelegate Bus Interface
//
////////////////////////////////////////////////////////////////////////////////

@protocol FileTransferDelegate <AJNBusInterface>


// signals
//
- (void)sendTransferFileNamed:(NSString*)name currentIndex:(NSNumber*)curr fileData:(AJNMessageArgument*)data inSession:(AJNSessionId)sessionId toDestination:(NSString*)destinationPath flags:(uint8_t)flags;


@end

////////////////////////////////////////////////////////////////////////////////

    
////////////////////////////////////////////////////////////////////////////////
//
// FileTransferDelegate Signal Handler Protocol
//
////////////////////////////////////////////////////////////////////////////////

@protocol FileTransferDelegateSignalHandler <AJNSignalHandler>

// signals
//
- (void)didReceiveTransferFileNamed:(NSString*)name currentIndex:(NSNumber*)curr fileData:(AJNMessageArgument*)data inSession:(AJNSessionId)sessionId fromSender:(NSString*)sender;


@end

@interface AJNBusAttachment(FileTransferDelegate)

- (void)registerFileTransferDelegateSignalHandler:(id<FileTransferDelegateSignalHandler>)signalHandler;

@end

////////////////////////////////////////////////////////////////////////////////
    

////////////////////////////////////////////////////////////////////////////////
//
//  AJNFileTransferObject Bus Object superclass
//
////////////////////////////////////////////////////////////////////////////////

@interface AJNFileTransferObject : AJNBusObject<FileTransferDelegate>

// properties
//


// methods
//


// signals
//
- (void)sendTransferFileNamed:(NSString*)name currentIndex:(NSNumber*)curr fileData:(AJNMessageArgument*)data inSession:(AJNSessionId)sessionId toDestination:(NSString*)destinationPath flags:(uint8_t)flags;


@end

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//
//  FileTransferObject Proxy
//
////////////////////////////////////////////////////////////////////////////////

@interface FileTransferObjectProxy : AJNProxyBusObject

// properties
//


// methods
//


@end

////////////////////////////////////////////////////////////////////////////////
