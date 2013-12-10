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

#import "AJNBusController.h"

////////////////////////////////////////////////////////////////////////////////
//
//  AJNClientDelegate protocol
//

@protocol AJNClientDelegate <AJNBusControllerDelegate>

- (AJNProxyBusObject *)proxyObjectOnBus:(AJNBusAttachment *)bus inSession:(AJNSessionId)sessionId;
- (void)shouldUnloadProxyObjectOnBus:(AJNBusAttachment *)bus;

@optional

- (void)didJoinInSession:(AJNSessionId)sessionId withService:(NSString *)serviceName;

@end
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  AJNClientController - abstracts and hides most of the boilerplate needed to
//  interact
//

@interface AJNClientController : NSObject<AJNBusController>

@property (nonatomic, weak) id<AJNClientDelegate> delegate;

@end
////////////////////////////////////////////////////////////////////////////////