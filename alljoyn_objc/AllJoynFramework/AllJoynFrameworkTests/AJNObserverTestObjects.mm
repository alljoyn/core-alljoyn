/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/
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
//  AJNObserverTestObjects.mm
//
////////////////////////////////////////////////////////////////////////////////

#import <alljoyn/BusAttachment.h>
#import <alljoyn/BusObject.h>
#import "AJNBusObjectImpl.h"
#import "AJNInterfaceDescription.h"
#import "AJNMessageArgument.h"
#import "AJNSignalHandlerImpl.h"

#import "ObserverTestObjects.h"

using namespace ajn;


@interface AJNMessageArgument(Private)

/**
 * Helper to return the C++ API object that is encapsulated by this objective-c class
 */
@property (nonatomic, readonly) MsgArg *msgArg;

@end


////////////////////////////////////////////////////////////////////////////////
//
//  C++ Bus Object class declaration for TestJustAImpl
//
////////////////////////////////////////////////////////////////////////////////
class TestJustAImpl : public AJNBusObjectImpl
{
private:
    
    
public:
    TestJustAImpl(BusAttachment &bus, const char *path, id<TestJustADelegate> aDelegate);

    
    
    // methods
    //
    void IdentifyA(const InterfaceDescription::Member* member, Message& msg);

    
    // signals
    //
    
};
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  C++ Bus Object implementation for TestJustAImpl
//
////////////////////////////////////////////////////////////////////////////////

TestJustAImpl::TestJustAImpl(BusAttachment &bus, const char *path, id<TestJustADelegate> aDelegate) : 
    AJNBusObjectImpl(bus,path,aDelegate)
{
    const InterfaceDescription* interfaceDescription = NULL;
    QStatus status;
    status = ER_OK;
    
    
    // Add the org.test.a interface to this object
    //
    interfaceDescription = bus.GetInterface("org.test.a");
    assert(interfaceDescription);
    AddInterface(*interfaceDescription, ANNOUNCED);

    
    // Register the method handlers for interface TestJustADelegate with the object
    //
    const MethodEntry methodEntriesForTestJustADelegate[] = {

        {
			interfaceDescription->GetMember("IdentifyA"), static_cast<MessageReceiver::MethodHandler>(&TestJustAImpl::IdentifyA)
		}
    
    };
    
    status = AddMethodHandlers(methodEntriesForTestJustADelegate, sizeof(methodEntriesForTestJustADelegate) / sizeof(methodEntriesForTestJustADelegate[0]));
    if (ER_OK != status) {
        NSLog(@"ERROR: An error occurred while adding method handlers for interface org.test.a to the interface description. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    

}


void TestJustAImpl::IdentifyA(const InterfaceDescription::Member *member, Message& msg)
{
    @autoreleasepool {
    
    
    
    // declare the output arguments
    //
    
	NSString* outArg0;
	NSString* outArg1;

    
    // call the Objective-C delegate method
    //
    
	[(id<TestJustADelegate>)delegate identifyAWithBusname:&outArg0 andPath:&outArg1  message:[[AJNMessage alloc] initWithHandle:&msg]];
            
        
    // formulate the reply
    //
    MsgArg outArgs[2];
    
    outArgs[0].Set("s", [outArg0 UTF8String]);

    outArgs[1].Set("s", [outArg1 UTF8String]);

    QStatus status = MethodReply(msg, outArgs, 2);
    if (ER_OK != status) {
        NSLog(@"ERROR: An error occurred when attempting to send a method reply for IdentifyA. %@", [AJNStatus descriptionForStatusCode:status]);
    }        
    
    
    }
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  C++ Bus Object class declaration for TestJustBImpl
//
////////////////////////////////////////////////////////////////////////////////
class TestJustBImpl : public AJNBusObjectImpl
{
private:
    
    
public:
    TestJustBImpl(BusAttachment &bus, const char *path, id<TestJustBDelegate> aDelegate);

    
    
    // methods
    //
    void IdentifyB(const InterfaceDescription::Member* member, Message& msg);

    
    // signals
    //
    
};
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  C++ Bus Object implementation for TestJustBImpl
//
////////////////////////////////////////////////////////////////////////////////

TestJustBImpl::TestJustBImpl(BusAttachment &bus, const char *path, id<TestJustBDelegate> aDelegate) : 
    AJNBusObjectImpl(bus,path,aDelegate)
{
    const InterfaceDescription* interfaceDescription = NULL;
    QStatus status;
    status = ER_OK;
    
    
    // Add the org.test.b interface to this object
    //
    interfaceDescription = bus.GetInterface("org.test.b");
    assert(interfaceDescription);
    AddInterface(*interfaceDescription, ANNOUNCED);

    
    // Register the method handlers for interface TestJustBDelegate with the object
    //
    const MethodEntry methodEntriesForTestJustBDelegate[] = {

        {
			interfaceDescription->GetMember("IdentifyB"), static_cast<MessageReceiver::MethodHandler>(&TestJustBImpl::IdentifyB)
		}
    
    };
    
    status = AddMethodHandlers(methodEntriesForTestJustBDelegate, sizeof(methodEntriesForTestJustBDelegate) / sizeof(methodEntriesForTestJustBDelegate[0]));
    if (ER_OK != status) {
        NSLog(@"ERROR: An error occurred while adding method handlers for interface org.test.b to the interface description. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    

}


void TestJustBImpl::IdentifyB(const InterfaceDescription::Member *member, Message& msg)
{
    @autoreleasepool {
    
    
    
    // declare the output arguments
    //
    
	NSString* outArg0;
	NSString* outArg1;

    
    // call the Objective-C delegate method
    //
    
	[(id<TestJustBDelegate>)delegate identifyBWithBusname:&outArg0 andPath:&outArg1  message:[[AJNMessage alloc] initWithHandle:&msg]];
            
        
    // formulate the reply
    //
    MsgArg outArgs[2];
    
    outArgs[0].Set("s", [outArg0 UTF8String]);

    outArgs[1].Set("s", [outArg1 UTF8String]);

    QStatus status = MethodReply(msg, outArgs, 2);
    if (ER_OK != status) {
        NSLog(@"ERROR: An error occurred when attempting to send a method reply for IdentifyB. %@", [AJNStatus descriptionForStatusCode:status]);
    }        
    
    
    }
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  C++ Bus Object class declaration for TestBothImpl
//
////////////////////////////////////////////////////////////////////////////////
class TestBothImpl : public AJNBusObjectImpl
{
private:
    
    
public:
    TestBothImpl(BusAttachment &bus, const char *path, id<TestBothDelegate, TestObjectABDelegateB> aDelegate);

    
    
    // methods
    //
    void IdentifyA(const InterfaceDescription::Member* member, Message& msg);
	void IdentifyB(const InterfaceDescription::Member* member, Message& msg);

    
    // signals
    //
    
};
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  C++ Bus Object implementation for TestBothImpl
//
////////////////////////////////////////////////////////////////////////////////

TestBothImpl::TestBothImpl(BusAttachment &bus, const char *path, id<TestBothDelegate, TestObjectABDelegateB> aDelegate) : 
    AJNBusObjectImpl(bus,path,aDelegate)
{
    const InterfaceDescription* interfaceDescription = NULL;
    QStatus status;
    status = ER_OK;
    
    
    // Add the org.test.a interface to this object
    //
    interfaceDescription = bus.GetInterface("org.test.a");
    assert(interfaceDescription);
    AddInterface(*interfaceDescription, ANNOUNCED);

    
    // Register the method handlers for interface TestBothDelegate with the object
    //
    const MethodEntry methodEntriesForTestBothDelegate[] = {

        {
			interfaceDescription->GetMember("IdentifyA"), static_cast<MessageReceiver::MethodHandler>(&TestBothImpl::IdentifyA)
		}
    
    };
    
    status = AddMethodHandlers(methodEntriesForTestBothDelegate, sizeof(methodEntriesForTestBothDelegate) / sizeof(methodEntriesForTestBothDelegate[0]));
    if (ER_OK != status) {
        NSLog(@"ERROR: An error occurred while adding method handlers for interface org.test.a to the interface description. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    
    // Add the org.test.b interface to this object
    //
    interfaceDescription = bus.GetInterface("org.test.b");
    assert(interfaceDescription);
    AddInterface(*interfaceDescription, ANNOUNCED);

    
    // Register the method handlers for interface TestObjectABDelegateB with the object
    //
    const MethodEntry methodEntriesForTestObjectABDelegateB[] = {

        {
			interfaceDescription->GetMember("IdentifyB"), static_cast<MessageReceiver::MethodHandler>(&TestBothImpl::IdentifyB)
		}
    
    };
    
    status = AddMethodHandlers(methodEntriesForTestObjectABDelegateB, sizeof(methodEntriesForTestObjectABDelegateB) / sizeof(methodEntriesForTestObjectABDelegateB[0]));
    if (ER_OK != status) {
        NSLog(@"ERROR: An error occurred while adding method handlers for interface org.test.b to the interface description. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    

}


void TestBothImpl::IdentifyA(const InterfaceDescription::Member *member, Message& msg)
{
    @autoreleasepool {
    
    
    
    // declare the output arguments
    //
    
	NSString* outArg0;
	NSString* outArg1;

    
    // call the Objective-C delegate method
    //
    
	[(id<TestBothDelegate>)delegate identifyAWithBusname:&outArg0 andPath:&outArg1  message:[[AJNMessage alloc] initWithHandle:&msg]];
            
        
    // formulate the reply
    //
    MsgArg outArgs[2];
    
    outArgs[0].Set("s", [outArg0 UTF8String]);

    outArgs[1].Set("s", [outArg1 UTF8String]);

    QStatus status = MethodReply(msg, outArgs, 2);
    if (ER_OK != status) {
        NSLog(@"ERROR: An error occurred when attempting to send a method reply for IdentifyA. %@", [AJNStatus descriptionForStatusCode:status]);
    }        
    
    
    }
}

void TestBothImpl::IdentifyB(const InterfaceDescription::Member *member, Message& msg)
{
    @autoreleasepool {
    
    
    
    // declare the output arguments
    //
    
	NSString* outArg0;
	NSString* outArg1;

    
    // call the Objective-C delegate method
    //
    
	[(id<TestObjectABDelegateB>)delegate identifyBWithBusname:&outArg0 andPath:&outArg1  message:[[AJNMessage alloc] initWithHandle:&msg]];
            
        
    // formulate the reply
    //
    MsgArg outArgs[2];
    
    outArgs[0].Set("s", [outArg0 UTF8String]);

    outArgs[1].Set("s", [outArg1 UTF8String]);

    QStatus status = MethodReply(msg, outArgs, 2);
    if (ER_OK != status) {
        NSLog(@"ERROR: An error occurred when attempting to send a method reply for IdentifyB. %@", [AJNStatus descriptionForStatusCode:status]);
    }        
    
    
    }
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  Objective-C Bus Object implementation for AJNTestJustA
//
////////////////////////////////////////////////////////////////////////////////

@implementation AJNTestJustA

@dynamic handle;



- (TestJustAImpl*)busObject
{
    return static_cast<TestJustAImpl*>(self.handle);
}

- (id)initWithBusAttachment:(AJNBusAttachment *)busAttachment onPath:(NSString *)path
{
    self = [super initWithBusAttachment:busAttachment onPath:path];
    if (self) {
        QStatus status;

        status = ER_OK;
        
        AJNInterfaceDescription *interfaceDescription;
        
    
        //
        // TestJustADelegate interface (org.test.a)
        //
        // create an interface description, or if that fails, get the interface as it was already created
        //
        interfaceDescription = [busAttachment createInterfaceWithName:@"org.test.a"];

    
        // add the methods to the interface description
        //
    
        status = [interfaceDescription addMethodWithName:@"IdentifyA" inputSignature:@"" outputSignature:@"ss" argumentNames:[NSArray arrayWithObjects:@"busname",@"path", nil]];
        
        if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
            @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add method to interface: IdentifyA" userInfo:nil];
        }
    
    

    
        [interfaceDescription activate];


        // create the internal C++ bus object
        //
        TestJustAImpl *busObject = new TestJustAImpl(*((ajn::BusAttachment*)busAttachment.handle), [path UTF8String], (id<TestJustADelegate>)self);
        
        self.handle = busObject;
        
      
    }
    return self;
}

- (void)dealloc
{
    TestJustAImpl *busObject = [self busObject];
    delete busObject;
    self.handle = nil;
}

    
- (void)identifyAWithBusname:(NSString**)busname andPath:(NSString**)path message:(AJNMessage *)methodCallMessage
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
//  Objective-C Proxy Bus Object implementation for TestJustA
//
////////////////////////////////////////////////////////////////////////////////

@interface TestJustAProxy(Private)

@property (nonatomic, strong) AJNBusAttachment *bus;

- (ProxyBusObject*)proxyBusObject;

@end

@implementation TestJustAProxy
    
- (void)identifyAWithBusname:(NSString**)busname andPath:(NSString**)path
{
    [self addInterfaceNamed:@"org.test.a"];
    
    // prepare the input arguments
    //
    
    Message reply(*((BusAttachment*)self.bus.handle));    
    MsgArg inArgs[0];
    

    // make the function call using the C++ proxy object
    //
    
    QStatus status = self.proxyBusObject->MethodCall("org.test.a", "IdentifyA", inArgs, 0, reply, 5000);
    if (ER_OK != status) {
        NSLog(@"ERROR: ProxyBusObject::MethodCall on org.test.a failed. %@", [AJNStatus descriptionForStatusCode:status]);
        
        return;
            
    }

    
    // pass the output arguments back to the caller
    //
    
        
    *busname = [NSString stringWithCString:reply->GetArg(0)->v_string.str encoding:NSUTF8StringEncoding];
        
    *path = [NSString stringWithCString:reply->GetArg(1)->v_string.str encoding:NSUTF8StringEncoding];
        

}

@end

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  Objective-C Bus Object implementation for AJNTestJustB
//
////////////////////////////////////////////////////////////////////////////////

@implementation AJNTestJustB

@dynamic handle;



- (TestJustBImpl*)busObject
{
    return static_cast<TestJustBImpl*>(self.handle);
}

- (id)initWithBusAttachment:(AJNBusAttachment *)busAttachment onPath:(NSString *)path
{
    self = [super initWithBusAttachment:busAttachment onPath:path];
    if (self) {
        QStatus status;

        status = ER_OK;
        
        AJNInterfaceDescription *interfaceDescription;
        
    
        //
        // TestJustBDelegate interface (org.test.b)
        //
        // create an interface description, or if that fails, get the interface as it was already created
        //
        interfaceDescription = [busAttachment createInterfaceWithName:@"org.test.b"];

    
        // add the methods to the interface description
        //
    
        status = [interfaceDescription addMethodWithName:@"IdentifyB" inputSignature:@"" outputSignature:@"ss" argumentNames:[NSArray arrayWithObjects:@"busname",@"path", nil]];
        
        if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
            @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add method to interface: IdentifyB" userInfo:nil];
        }
    
    

    
        [interfaceDescription activate];


        // create the internal C++ bus object
        //
        TestJustBImpl *busObject = new TestJustBImpl(*((ajn::BusAttachment*)busAttachment.handle), [path UTF8String], (id<TestJustBDelegate>)self);
        
        self.handle = busObject;
        
      
    }
    return self;
}

- (void)dealloc
{
    TestJustBImpl *busObject = [self busObject];
    delete busObject;
    self.handle = nil;
}

    
- (void)identifyBWithBusname:(NSString**)busname andPath:(NSString**)path message:(AJNMessage *)methodCallMessage
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
//  Objective-C Proxy Bus Object implementation for TestJustB
//
////////////////////////////////////////////////////////////////////////////////

@interface TestJustBProxy(Private)

@property (nonatomic, strong) AJNBusAttachment *bus;

- (ProxyBusObject*)proxyBusObject;

@end

@implementation TestJustBProxy
    
- (void)identifyBWithBusname:(NSString**)busname andPath:(NSString**)path
{
    [self addInterfaceNamed:@"org.test.b"];
    
    // prepare the input arguments
    //
    
    Message reply(*((BusAttachment*)self.bus.handle));    
    MsgArg inArgs[0];
    

    // make the function call using the C++ proxy object
    //
    
    QStatus status = self.proxyBusObject->MethodCall("org.test.b", "IdentifyB", inArgs, 0, reply, 5000);
    if (ER_OK != status) {
        NSLog(@"ERROR: ProxyBusObject::MethodCall on org.test.b failed. %@", [AJNStatus descriptionForStatusCode:status]);
        
        return;
            
    }

    
    // pass the output arguments back to the caller
    //
    
        
    *busname = [NSString stringWithCString:reply->GetArg(0)->v_string.str encoding:NSUTF8StringEncoding];
        
    *path = [NSString stringWithCString:reply->GetArg(1)->v_string.str encoding:NSUTF8StringEncoding];
        

}

@end

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  Objective-C Bus Object implementation for AJNTestBoth
//
////////////////////////////////////////////////////////////////////////////////

@implementation AJNTestBoth

@dynamic handle;



- (TestBothImpl*)busObject
{
    return static_cast<TestBothImpl*>(self.handle);
}

- (id)initWithBusAttachment:(AJNBusAttachment *)busAttachment onPath:(NSString *)path
{
    self = [super initWithBusAttachment:busAttachment onPath:path];
    if (self) {
        QStatus status;

        status = ER_OK;
        
        AJNInterfaceDescription *interfaceDescription;
        
    
        //
        // TestBothDelegate interface (org.test.a)
        //
        // create an interface description, or if that fails, get the interface as it was already created
        //
        interfaceDescription = [busAttachment createInterfaceWithName:@"org.test.a"];

    
        // add the methods to the interface description
        //
    
        status = [interfaceDescription addMethodWithName:@"IdentifyA" inputSignature:@"" outputSignature:@"ss" argumentNames:[NSArray arrayWithObjects:@"busname",@"path", nil]];
        
        if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
            @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add method to interface: IdentifyA" userInfo:nil];
        }
    
    

    
        [interfaceDescription activate];

        //
        // TestObjectABDelegateB interface (org.test.b)
        //
        // create an interface description, or if that fails, get the interface as it was already created
        //
        interfaceDescription = [busAttachment createInterfaceWithName:@"org.test.b"];

    
        // add the methods to the interface description
        //
    
        status = [interfaceDescription addMethodWithName:@"IdentifyB" inputSignature:@"" outputSignature:@"ss" argumentNames:[NSArray arrayWithObjects:@"busname",@"path", nil]];
        
        if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
            @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add method to interface: IdentifyB" userInfo:nil];
        }
    
    

    
        [interfaceDescription activate];


        // create the internal C++ bus object
        //
        TestBothImpl *busObject = new TestBothImpl(*((ajn::BusAttachment*)busAttachment.handle), [path UTF8String], (id<TestBothDelegate, TestObjectABDelegateB>)self);
        
        self.handle = busObject;
        
      
    }
    return self;
}

- (void)dealloc
{
    TestBothImpl *busObject = [self busObject];
    delete busObject;
    self.handle = nil;
}

    
- (void)identifyAWithBusname:(NSString**)busname andPath:(NSString**)path message:(AJNMessage *)methodCallMessage
{
    //
    // GENERATED CODE - DO NOT EDIT
    //
    // Create a category or subclass in separate .h/.m files
    @throw([NSException exceptionWithName:@"NotImplementedException" reason:@"You must override this method in a subclass" userInfo:nil]);
}

- (void)identifyBWithBusname:(NSString**)busname andPath:(NSString**)path message:(AJNMessage *)methodCallMessage
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
//  Objective-C Proxy Bus Object implementation for TestBoth
//
////////////////////////////////////////////////////////////////////////////////

@interface TestBothProxy(Private)

@property (nonatomic, strong) AJNBusAttachment *bus;

- (ProxyBusObject*)proxyBusObject;

@end

@implementation TestBothProxy
    
- (void)identifyAWithBusname:(NSString**)busname andPath:(NSString**)path
{
    [self addInterfaceNamed:@"org.test.a"];
    
    // prepare the input arguments
    //
    
    Message reply(*((BusAttachment*)self.bus.handle));    
    MsgArg inArgs[0];
    

    // make the function call using the C++ proxy object
    //
    
    QStatus status = self.proxyBusObject->MethodCall("org.test.a", "IdentifyA", inArgs, 0, reply, 5000);
    if (ER_OK != status) {
        NSLog(@"ERROR: ProxyBusObject::MethodCall on org.test.a failed. %@", [AJNStatus descriptionForStatusCode:status]);
        
        return;
            
    }

    
    // pass the output arguments back to the caller
    //
    
        
    *busname = [NSString stringWithCString:reply->GetArg(0)->v_string.str encoding:NSUTF8StringEncoding];
        
    *path = [NSString stringWithCString:reply->GetArg(1)->v_string.str encoding:NSUTF8StringEncoding];
        

}

- (void)identifyBWithBusname:(NSString**)busname andPath:(NSString**)path
{
    [self addInterfaceNamed:@"org.test.b"];
    
    // prepare the input arguments
    //
    
    Message reply(*((BusAttachment*)self.bus.handle));    
    MsgArg inArgs[0];
    

    // make the function call using the C++ proxy object
    //
    
    QStatus status = self.proxyBusObject->MethodCall("org.test.b", "IdentifyB", inArgs, 0, reply, 5000);
    if (ER_OK != status) {
        NSLog(@"ERROR: ProxyBusObject::MethodCall on org.test.b failed. %@", [AJNStatus descriptionForStatusCode:status]);
        
        return;
            
    }

    
    // pass the output arguments back to the caller
    //
    
        
    *busname = [NSString stringWithCString:reply->GetArg(0)->v_string.str encoding:NSUTF8StringEncoding];
        
    *path = [NSString stringWithCString:reply->GetArg(1)->v_string.str encoding:NSUTF8StringEncoding];
        

}

@end

////////////////////////////////////////////////////////////////////////////////
