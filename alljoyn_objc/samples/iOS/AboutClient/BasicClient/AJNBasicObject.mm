////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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
//  AJNBasicObject.mm
//
////////////////////////////////////////////////////////////////////////////////

#import <alljoyn/BusAttachment.h>
#import <alljoyn/BusObject.h>
#import "AJNBusObjectImpl.h"
#import "AJNInterfaceDescription.h"
#import "AJNSignalHandlerImpl.h"

#import "BasicObject.h"

using namespace ajn;

////////////////////////////////////////////////////////////////////////////////
//
//  C++ Bus Object class declaration for BasicObjectImpl
//
////////////////////////////////////////////////////////////////////////////////
class BasicObjectImpl : public AJNBusObjectImpl
{
private:

public:
    BasicObjectImpl(BusAttachment &bus, const char *path, id<BasicStringsDelegate> aDelegate);


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
//  C++ Bus Object implementation for BasicObjectImpl
//
////////////////////////////////////////////////////////////////////////////////

BasicObjectImpl::BasicObjectImpl(BusAttachment &bus, const char *path, id<BasicStringsDelegate> aDelegate) :
    AJNBusObjectImpl(bus,path,aDelegate)
{
    const InterfaceDescription* interfaceDescription = NULL;
    QStatus status = ER_OK;


    // Add the org.alljoyn.bus.sample.strings interface to this object
    //
    interfaceDescription = bus.GetInterface([@"org.alljoyn.bus.sample" UTF8String]);
    assert(interfaceDescription);
    AddInterface(*interfaceDescription, ANNOUNCED);


    // Register the method handlers for interface BasicStringsDelegate with the object
    //
    const MethodEntry methodEntriesForBasicStringsDelegate[] = {

        {
			interfaceDescription->GetMember("cat"), static_cast<MessageReceiver::MethodHandler>(&BasicObjectImpl::Concatentate)
		},
    };

    status = AddMethodHandlers(methodEntriesForBasicStringsDelegate, sizeof(methodEntriesForBasicStringsDelegate) / sizeof(methodEntriesForBasicStringsDelegate[0]));
    if (ER_OK != status) {
        // TODO: perform error checking here
    }

}

void BasicObjectImpl::Concatentate(const InterfaceDescription::Member *member, Message& msg)
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

	outArg0 = [(id<BasicStringsDelegate>)delegate concatenateString:[NSString stringWithCString:inArg0.c_str() encoding:NSUTF8StringEncoding] withString:[NSString stringWithCString:inArg1.c_str() encoding:NSUTF8StringEncoding]];


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
//  Objective-C Bus Object implementation for AJNBasicObject
//
////////////////////////////////////////////////////////////////////////////////

@implementation AJNBasicObject

@dynamic handle;


- (BasicObjectImpl*)busObject
{
    return static_cast<BasicObjectImpl*>(self.handle);
}

- (id)initWithBusAttachment:(AJNBusAttachment *)busAttachment onPath:(NSString *)path
{
    self = [super initWithBusAttachment:busAttachment onPath:path];
    if (self) {
        QStatus status = ER_OK;

        AJNInterfaceDescription *interfaceDescription;


        //
        // BasicStringsDelegate interface (org.alljoyn.bus.sample.strings)
        //
        // create an interface description
        //
        interfaceDescription = [busAttachment createInterfaceWithName:@"org.alljoyn.bus.sample" enableSecurity:NO];


        // add the methods to the interface description
        //

        status = [interfaceDescription addMethodWithName:@"cat" inputSignature:@"ss" outputSignature:@"s" argumentNames:[NSArray arrayWithObjects:@"str1",@"str2",@"outStr", nil]];

        if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
            @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add method to interface: Concatentate" userInfo:nil];
        }


        [interfaceDescription activate];

        // create the internal C++ bus object
        //
        BasicObjectImpl *busObject = new BasicObjectImpl(*((ajn::BusAttachment*)busAttachment.handle), [path UTF8String], (id<BasicStringsDelegate>)self);

        self.handle = busObject;
    }
    return self;
}

- (void)dealloc
{
    BasicObjectImpl *busObject = [self busObject];
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
//  Objective-C Proxy Bus Object implementation for BasicObject
//
////////////////////////////////////////////////////////////////////////////////

@interface BasicObjectProxy(Private)

@property (nonatomic, strong) AJNBusAttachment *bus;

- (ProxyBusObject*)proxyBusObject;

@end

@implementation BasicObjectProxy

- (NSString*)concatenateString:(NSString*)str1 withString:(NSString*)str2
{
    [self addInterfaceNamed:@"org.alljoyn.bus.sample"];

    // prepare the input arguments
    //

    Message reply(*((BusAttachment*)self.bus.handle));
    MsgArg inArgs[2];

    inArgs[0].Set("s", [str1 UTF8String]);

    inArgs[1].Set("s", [str2 UTF8String]);


    // make the function call using the C++ proxy object
    //
    QStatus status = self.proxyBusObject->MethodCall([@"org.alljoyn.bus.sample" UTF8String], "cat", inArgs, 2, reply, 5000);
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
