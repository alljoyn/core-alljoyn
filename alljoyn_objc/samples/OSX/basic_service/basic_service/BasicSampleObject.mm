////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2012-2014, AllSeen Alliance. All rights reserved.
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

#import "BasicSampleObject.h"
#import "BasicSampleObjectImpl.h"
#import "AJNInterfaceDescription.h"

@interface InterfaceTranslator : NSObject<AJNTranslator>

- (size_t)numTargetLanguages;
- (NSString*)getTargetLanguage:(size_t)index;
- (NSString*)translateText:(NSString*)text from:(NSString*)fromLang to:(NSString*)toLang;

@end

@implementation InterfaceTranslator
- (size_t)numTargetLanguages
{
    return 1;
}

- (NSString*)getTargetLanguage:(size_t)index
{
    return @"en";
}

- (NSString*)translateText:(NSString*)text from:(NSString*)fromLang to:(NSString*)toLang
{
    if(![toLang isEqualToString:(@"en")])
    {
        return nil;
    }
    if([text isEqualToString:@"Isthay siay naay nterfacenay"])
    {
        return @"This is an interface";
    }
    if([text isEqualToString:@"IsThay siay ethay atcay"])
    {
        return @"This is the cat";
    }
    if([text isEqualToString:@"IsThay siay ethay esponsray"])
    {
        return @"This is the response";
    }
    return nil;
}

@end

@implementation MyBasicSampleObject

@synthesize delegate = _delegate;

- (BasicSampleObjectImpl*)busObject
{
    return (BasicSampleObjectImpl*)self.handle;
}

- (id)initWithBusAttachment:(AJNBusAttachment *)busAttachment onPath:(NSString *)path
{
    self = [super initWithBusAttachment:busAttachment onPath:path];
    if (self) {
        BOOL result;
        
        AJNInterfaceDescription *interfaceDescription;
        
        // create an interface description and add the concatenate method to it
        //
        interfaceDescription = [busAttachment createInterfaceWithName:kBasicObjectInterfaceName];

        result = [interfaceDescription addMethodWithName:kBasicObjectMethodName inputSignature:@"ss" outputSignature:@"s" argumentNames:[NSArray arrayWithObjects:@"str1", @"str2", @"outStr", nil]];
        
        if (result != ER_OK) {
            [self.delegate didReceiveStatusUpdateMessage:@"Failed to create interface 'org.alljoyn.Bus.method_sample'\n"];
            
            @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add method to interface" userInfo:nil];
        }
        [self.delegate didReceiveStatusUpdateMessage:@"Interface Created.\n"];        
 
        [interfaceDescription setDescriptionLanguage:@"pig"];
        [interfaceDescription setDescription:@"Isthay siay naay nterfacenay"];
        [interfaceDescription setMemberDescription:@"IsThay siay ethay atcay" forMemberWithName:@"cat" sessionlessSignal:FALSE];
        [interfaceDescription setArgDescription:@"IsThay siay ethay esponsray" forArgument:@"outStr" ofMember:@"cat"];
        [interfaceDescription setDescriptionTranslator:[InterfaceTranslator alloc]];
        
        [interfaceDescription activate];
        
        // create the internal C++ bus object
        //
        BasicSampleObjectImpl *busObject = new BasicSampleObjectImpl(*((ajn::BusAttachment*)busAttachment.handle), [path UTF8String], (id<MyMethodSample>)self);
        
        self.handle = busObject;
    }
    return self;
}

- (void)dealloc
{
    BasicSampleObjectImpl *busObject = [self busObject];
    delete busObject;
    self.handle = nil;
}

- (void)objectWasRegistered
{
    [self.delegate didReceiveStatusUpdateMessage:@"MyBasicSampleObject was registered.\n"];
}

@end
