////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013-2014, AllSeen Alliance. All rights reserved.
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

- (void)setDescriptionTranslator:(id<AJNTranslator>)translator
{
    if(self.translator)
    {
        delete (ajn::Translator*)self.translator;
    }
    self.translator = new AJNTranslatorImpl(translator);
    [self busObject]->SetDescriptionTranslator((ajn::Translator*)self.translator);
}

-(void)dealloc
{
    if(self.translator)
    {
        delete (AJNTranslatorImpl*)self.translator;
    }
}

@end
