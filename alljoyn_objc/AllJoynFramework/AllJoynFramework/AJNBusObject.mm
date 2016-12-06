////////////////////////////////////////////////////////////////////////////////
//    Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
//    Source Project (AJOSP) Contributors and others.
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
//     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
//     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
//     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
//     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
//     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
//     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
//     PERFORMANCE OF THIS SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#import <alljoyn/BusObject.h>
#import <alljoyn/InterfaceDescription.h>
#import <alljoyn/MsgArg.h>
#import <alljoyn/Message.h>
#import "AJNBusAttachment.h"
#import "AJNBusObject.h"
#import "AJNBusObjectImpl.h"
#import "AJNInterfaceDescription.h"
#import "AJNMessageArgument.h"
#import "AJNTranslatorImpl.h"

using namespace ajn;

@interface AJNMessageArgument(Private)

@property (nonatomic, readonly) MsgArg *msgArg;

@end

@interface AJNBusObject()

/**
 * The bus attachment this object is associated with.
 */
@property (nonatomic, weak) AJNBusAttachment *bus;

@end

using namespace ajn;

@interface AJNBusAttachment(Private)

@property (nonatomic, readonly) BusAttachment *busAttachment;

@end


@interface AJNInterfaceMember(Private)

@property (nonatomic, readonly) ajn::InterfaceDescription::Member *member;

@end

@interface AJNInterfaceProperty(Private)

@property (nonatomic, readonly) ajn::InterfaceDescription::Property *property;

@end

@implementation AJNBusObject

@synthesize bus = _bus;

/**
 * Helper to return the C++ API object that is encapsulated by this objective-c class
 */
- (AJNBusObjectImpl*)busObject
{
    return static_cast<AJNBusObjectImpl*>(self.handle);
}

- (NSString*)path
{
    return [NSString stringWithCString:self.busObject->GetPath() encoding:NSUTF8StringEncoding];
}

- (NSString*)name
{
    return [NSString stringWithCString:self.busObject->GetName().c_str() encoding:NSUTF8StringEncoding];
}

-(BOOL)isSecure
{
    return self.busObject->IsSecure() ? YES : NO;
}

- (id)initWithPath:(NSString*)path
{
    self = [super init];
    if (self) {
        // initialization
    }
    return self;
}

- (id)initWithBusAttachment:(AJNBusAttachment*)busAttachment onPath:(NSString*)path
{
    self = [super init];
    if (self) {
        self.bus = busAttachment;
    }
    return self;
}

- (void)objectWasRegistered
{
    NSLog(@"AJNBusObject::ObjectRegistered called.");
}

- (void)emitPropertyWithName:(NSString*)propertyName onInterfaceWithName:(NSString*)interfaceName changedToValue:(AJNMessageArgument*)value inSession:(AJNSessionId)sessionId
{
    self.busObject->EmitPropChanged([interfaceName UTF8String], [propertyName UTF8String], *value.msgArg, sessionId);
}

- (void)emitPropertyWithName:(NSString *)propertyName onInterfaceWithName:(NSString *)interfaceName changedToValue:(AJNMessageArgument *)value inSession:(AJNSessionId)sessionId withFlags:(uint8_t)flags
{
    self.busObject->EmitPropChanged([interfaceName UTF8String], [propertyName UTF8String], *value.msgArg, sessionId, flags);
}

- (QStatus)emitPropertiesWithNames:(NSArray*)propNames onInterfaceWithName:(NSString*)ifcName inSession:(AJNSessionId)sessionId withFlags:(uint8_t)flags
{
    QStatus status = ER_OK;
    const char **properties = new const char*[propNames.count];
    for (int i = 0; i < propNames.count; i++) {
        properties[i] = [[propNames objectAtIndex:i] UTF8String];
    }
    status = self.busObject->EmitPropChanged([ifcName UTF8String], properties, propNames.count, sessionId, flags);
    
    delete[] properties;
    
    return status;
}

- (QStatus)cancelSessionlessMessageWithSerial:(uint32_t)serialNumber
{
    return self.busObject->CancelSessionlessMessage(serialNumber);
}

- (QStatus)cancelSessionlessMessageWithMessage:(const AJNMessage *)message
{
    return self.busObject->CancelSessionlessMessage(*(Message *)message.handle);
}

- (void)setDescription:(NSString *)description inLanguage:(NSString *)language
{
    [self busObject]->SetDescription([language UTF8String], [description UTF8String]);
}

- (QStatus)signal:(NSString *)destination inSession:(AJNSessionId)sessionId withSignal:(AJNInterfaceMember *)signalMmbr
{
    ajn::InterfaceDescription::Member ifcMember = *(signalMmbr.member);
    return self.busObject->Signal([destination UTF8String], sessionId, ifcMember);
}

- (QStatus)signal:(NSString*)destination inSession:(AJNSessionId)sessionId withSignal:(AJNInterfaceMember*)signalMmbr withArgs:(NSArray*)args
{
    QStatus status = ER_OK;
    ajn::InterfaceDescription::Member ifcMember = *(signalMmbr.member);
    MsgArg * pArgs = new MsgArg[args.count];
    
    for (int i = 0; i < args.count; i++) {
        pArgs[i] = *[[args objectAtIndex:i] msgArg];
    }
    status = self.busObject->Signal([destination UTF8String], sessionId, ifcMember, pArgs);
    delete [] pArgs;
    return status;
}


- (QStatus)signal:(NSString *)destination inSession:(AJNSessionId)sessionId withSignal:(AJNInterfaceMember *)signalMmbr withArgs:(NSArray *)args ttl:(uint16_t)timeToLive withFlags:(uint8_t)flags withMsg:(AJNMessage **)msg
{
    QStatus status = ER_OK;
    ajn::InterfaceDescription::Member ifcMember = *(signalMmbr.member);
    MsgArg * pArgs = new MsgArg[args.count];
    
    for (int i = 0; i < args.count; i++) {
        pArgs[i] = *[[args objectAtIndex:i] msgArg];
    }
    Message *replyMsg = new Message(*self.bus.busAttachment);
    status = self.busObject->Signal([destination UTF8String], sessionId, ifcMember, pArgs, args.count, timeToLive, flags, replyMsg);
    delete [] pArgs;
    *msg = [[AJNMessage alloc] initWithHandle:replyMsg];
    return status;
}

- (void)setDescriptionTranslator:(id<AJNTranslator>)translator
{
    if(self.translator)
    {
        delete (ajn::Translator*)self.translator;
    }
    self.translator = new AJNTranslatorImpl(translator);
    [self busObject]->SetDescriptionTranslator((ajn::Translator*)self.translator);
}

- (QStatus)setAnnounceFlagForInterface:(AJNInterfaceDescription *)iface value:(AJNAnnounceFlag)flag
{
    return self.busObject->SetAnnounceFlag((ajn::InterfaceDescription *)iface.handle, (BusObject::AnnounceFlag)flag);
}

-(void)dealloc
{
    if(self.translator)
    {
        delete (AJNTranslatorImpl*)self.translator;
    }
}

@end