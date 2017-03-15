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
//  AJNAboutSampleObject.mm
//
////////////////////////////////////////////////////////////////////////////////

#import <alljoyn/BusAttachment.h>
#import <alljoyn/BusObject.h>
#import "AJNBusObjectImpl.h"
#import "AJNInterfaceDescription.h"
#import "AJNSignalHandlerImpl.h"

#import "AboutSampleObject.h"

using namespace ajn;

////////////////////////////////////////////////////////////////////////////////
//
//  C++ Bus Object class declaration for AboutSampleObjectImpl
//
////////////////////////////////////////////////////////////////////////////////
class AboutSampleObjectImpl : public AJNBusObjectImpl
{
private:

public:
    AboutSampleObjectImpl(BusAttachment &bus, const char *path, id<AboutSampleStringsDelegate> aDelegate);


    // properties
    //

    // methods
    //
    void Concatentate(const InterfaceDescription::Member* member, Message& msg);

    // signals
    //

};
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  C++ Bus Object implementation for AboutSampleObjectImpl
//
////////////////////////////////////////////////////////////////////////////////

AboutSampleObjectImpl::AboutSampleObjectImpl(BusAttachment &bus, const char *path, id<AboutSampleStringsDelegate> aDelegate) :
    AJNBusObjectImpl(bus,path,aDelegate)
{
    const InterfaceDescription* interfaceDescription = NULL;
    QStatus status = ER_OK;


    // Add the org.alljoyn.bus.sample.strings interface to this object
    //
    interfaceDescription = bus.GetInterface([@"com.example.about.feature.interface.sample" UTF8String]);
    assert(interfaceDescription);
    AddInterface(*interfaceDescription, ANNOUNCED);


    // Register the method handlers for interface AboutSampleStringsDelegate with the object
    //
    const MethodEntry methodEntriesForAboutSampleStringsDelegate[] = {

        {
			interfaceDescription->GetMember("cat"), static_cast<MessageReceiver::MethodHandler>(&AboutSampleObjectImpl::Concatentate)
		},
    };

    status = AddMethodHandlers(methodEntriesForAboutSampleStringsDelegate, sizeof(methodEntriesForAboutSampleStringsDelegate) / sizeof(methodEntriesForAboutSampleStringsDelegate[0]));
    if (ER_OK != status) {
        // TODO: perform error checking here
    }

}

void AboutSampleObjectImpl::Concatentate(const InterfaceDescription::Member *member, Message& msg)
{
    @autoreleasepool {




    // get all input arguments
    //

    qcc::String inArg0 = msg->GetArg(0)->v_string.str;

    qcc::String inArg1 = msg->GetArg(1)->v_string.str;

    // declare the output arguments
    //

	NSString* outArg0;


    // call the Objective-C delegate method
    //

	outArg0 = [(id<AboutSampleStringsDelegate>)delegate concatenateString:[NSString stringWithCString:inArg0.c_str() encoding:NSUTF8StringEncoding] withString:[NSString stringWithCString:inArg1.c_str() encoding:NSUTF8StringEncoding]];


    // formulate the reply
    //
    MsgArg outArgs[1];

    outArgs[0].Set("s", [outArg0 UTF8String]);

    QStatus status = MethodReply(msg, outArgs, 1);
    if (ER_OK != status) {
        // TODO: exception handling
    }


    }
}


////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  Objective-C Bus Object implementation for AJNAboutSampleObject
//
////////////////////////////////////////////////////////////////////////////////

@implementation AJNAboutSampleObject

@dynamic handle;


- (AboutSampleObjectImpl*)busObject
{
    return static_cast<AboutSampleObjectImpl*>(self.handle);
}

- (id)initWithBusAttachment:(AJNBusAttachment *)busAttachment onPath:(NSString *)path
{
    self = [super initWithBusAttachment:busAttachment onPath:path];
    if (self) {
        QStatus status = ER_OK;

        AJNInterfaceDescription *interfaceDescription;


        //
        // AboutSampleStringsDelegate interface (org.alljoyn.bus.sample.strings)
        //
        // create an interface description
        //
        interfaceDescription = [busAttachment createInterfaceWithName:@"com.example.about.feature.interface.sample" enableSecurity:NO];


        // add the methods to the interface description
        //

        status = [interfaceDescription addMethodWithName:@"cat" inputSignature:@"ss" outputSignature:@"s" argumentNames:[NSArray arrayWithObjects:@"str1",@"str2",@"outStr", nil]];

        if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
            @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add method to interface: Concatentate" userInfo:nil];
        }


        [interfaceDescription activate];

        // create the internal C++ bus object
        //
        AboutSampleObjectImpl *busObject = new AboutSampleObjectImpl(*((ajn::BusAttachment*)busAttachment.handle), [path UTF8String], (id<AboutSampleStringsDelegate>)self);

        self.handle = busObject;
    }
    return self;
}

- (void)dealloc
{
    AboutSampleObjectImpl *busObject = [self busObject];
    delete busObject;
    self.handle = nil;
}


- (NSString*)concatenateString:(NSString*)str1 withString:(NSString*)str2
{
    //
    // GENERATED CODE - DO NOT EDIT
    //
    // Create a category or subclass in separate .h/.m files
    @throw([NSException exceptionWithName:@"NotImplementedException" reason:@"You must override this method in a subclass" userInfo:nil]);
}

@end

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  Objective-C Proxy Bus Object implementation for AboutSampleObject
//
////////////////////////////////////////////////////////////////////////////////

@interface AboutSampleObjectProxy(Private)

@property (nonatomic, strong) AJNBusAttachment *bus;

- (ProxyBusObject*)proxyBusObject;

@end

@implementation AboutSampleObjectProxy

- (NSString*)concatenateString:(NSString*)str1 withString:(NSString*)str2
{
    [self addInterfaceNamed:@"com.example.about.feature.interface.sample"];

    // prepare the input arguments
    //

    Message reply(*((BusAttachment*)self.bus.handle));
    MsgArg inArgs[2];

    inArgs[0].Set("s", [str1 UTF8String]);

    inArgs[1].Set("s", [str2 UTF8String]);


    // make the function call using the C++ proxy object
    //
    QStatus status = self.proxyBusObject->MethodCall([@"com.example.about.feature.interface.sample" UTF8String], "cat", inArgs, 2, reply, 5000);
    if (ER_OK != status) {
        NSLog(@"ERROR: ProxyBusObject::MethodCall on org.alljoyn.bus.sample failed. %@", [AJNStatus descriptionForStatusCode:status]);

        return nil;

    }


    // pass the output arguments back to the caller
    //

    return [NSString stringWithCString:reply->GetArg()->v_string.str encoding:NSUTF8StringEncoding];


}

@end

////////////////////////////////////////////////////////////////////////////////
