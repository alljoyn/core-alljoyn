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
#import "AJNCBusObject.h"
#import "AJNSessionOptions.h"
#import "AJNBusAttachment.h"

typedef enum {
    kAJNCConversationTypeRemoteHost = 0,
    kAJNCConversationTypeLocalHost = 1
} AJNCConversationType;

@interface AJNCConversation : NSObject<AJNChatReceiver>

@property (nonatomic, strong) NSString *name;
@property (nonatomic, readonly) NSString *displayName;
@property (nonatomic) AJNSessionId identifier;
@property (nonatomic, strong) AJNCBusObject *chatObject;
@property (nonatomic, strong) NSMutableArray *messages;
@property (nonatomic) AJNCConversationType type;
@property (nonatomic) NSInteger totalParticipants;

- (id)initWithName:(NSString *)name identifier:(AJNSessionId)sessionId busObject:(AJNCBusObject *)chatObject;

@end
