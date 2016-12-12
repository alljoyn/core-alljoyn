////////////////////////////////////////////////////////////////////////////////
// Copyright AllSeen Alliance. All rights reserved.
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
//  AJNSecureObject.mm
//
////////////////////////////////////////////////////////////////////////////////

#import <alljoyn/BusAttachment.h>
#import <alljoyn/BusObject.h>
#import "AJNBusObjectImpl.h"
#import "AJNInterfaceDescription.h"
#import "AJNMessageArgument.h"
#import "AJNSignalHandlerImpl.h"

#import "SecureObject.h"

using namespace ajn;


@interface AJNMessageArgument(Private)

/**
 * Helper to return the C++ API object that is encapsulated by this objective-c class
 */
@property (nonatomic, readonly) MsgArg *msgArg;

@end


////////////////////////////////////////////////////////////////////////////////
//
//  C++ Bus Object class declaration for SecureObjectImpl
//
////////////////////////////////////////////////////////////////////////////////
class SecureObjectImpl : public AJNBusObjectImpl
{
private:
    
    
public:
    SecureObjectImpl(const char *path, id<SecureObjectDelegate> aDelegate);
    SecureObjectImpl(BusAttachment &bus, const char *path, id<SecureObjectDelegate> aDelegate);

    virtual QStatus AddInterfacesAndHandlers(BusAttachment &bus);

    
    
    // methods
    //
    void concatenate(const InterfaceDescription::Member* member, Message& msg);

    
    // signals
    //
    
};
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  C++ Bus Object implementation for SecureObjectImpl
//
////////////////////////////////////////////////////////////////////////////////

SecureObjectImpl::SecureObjectImpl(const char *path, id<SecureObjectDelegate> aDelegate) :
    AJNBusObjectImpl(path,aDelegate)
{
    // Intentionally empty
}

SecureObjectImpl::SecureObjectImpl(BusAttachment &bus, const char *path, id<SecureObjectDelegate> aDelegate) :
    AJNBusObjectImpl(bus,path,aDelegate)
{
    AddInterfacesAndHandlers(bus);
}

QStatus SecureObjectImpl::AddInterfacesAndHandlers(BusAttachment &bus)
{
    const InterfaceDescription* interfaceDescription = NULL;
    QStatus status;
    status = ER_OK;
    
    
    // Add the org.alljoyn.bus.unittests.secure.SecureInterface interface to this object
    //
    interfaceDescription = bus.GetInterface("org.alljoyn.bus.unittests.secure.SecureInterface");
    assert(interfaceDescription);
    status = AddInterface(*interfaceDescription);
    if (ER_OK != status) {
        NSLog(@"ERROR: An error occurred while adding the interface org.alljoyn.bus.unittests.secure.SecureInterface.%@", [AJNStatus descriptionForStatusCode:status]);
    }
    
    // Register the method handlers for interface SecureObjectDelegate with the object
    //
    const MethodEntry methodEntriesForSecureObjectDelegate[] = {

        {
			interfaceDescription->GetMember("concatenate"), static_cast<MessageReceiver::MethodHandler>(&SecureObjectImpl::concatenate)
		}
    
    };
    
    status = AddMethodHandlers(methodEntriesForSecureObjectDelegate, sizeof(methodEntriesForSecureObjectDelegate) / sizeof(methodEntriesForSecureObjectDelegate[0]));
    if (ER_OK != status) {
        NSLog(@"ERROR: An error occurred while adding method handlers for interface org.alljoyn.bus.unittests.secure.SecureInterface to the interface description. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    
    return status;
}


void SecureObjectImpl::concatenate(const InterfaceDescription::Member *member, Message& msg)
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
    
	outArg0 = [(id<SecureObjectDelegate>)delegate concatenateString:[NSString stringWithCString:inArg0.c_str() encoding:NSUTF8StringEncoding] withString:[NSString stringWithCString:inArg1.c_str() encoding:NSUTF8StringEncoding] message:[[AJNMessage alloc] initWithHandle:&msg]];
            
        
    // formulate the reply
    //
    MsgArg outArgs[1];
    
    outArgs[0].Set("s", [outArg0 UTF8String]);
        
    QStatus status = MethodReply(msg, outArgs, 1);
    if (ER_OK != status) {
        NSLog(@"ERROR: An error occurred when attempting to send a method reply for concatenate. %@", [AJNStatus descriptionForStatusCode:status]);
    }  
        
    
    }
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  Objective-C Bus Object implementation for AJNSecureObject
//
////////////////////////////////////////////////////////////////////////////////

@interface AJNSecureObject()
/**
* The bus attachment this object is associated with.
*/
@property (nonatomic, weak) AJNBusAttachment *bus;

@end

@implementation AJNSecureObject

@dynamic handle;
@synthesize bus = _bus;



- (SecureObjectImpl*)busObject
{
    return static_cast<SecureObjectImpl*>(self.handle);
}

- (QStatus)registerInterfacesWithBus:(AJNBusAttachment *)busAttachment
{
    QStatus status;

    status = [self activateInterfacesWithBus: busAttachment];

    self.busObject->AddInterfacesAndHandlers(*((ajn::BusAttachment*)busAttachment.handle));

    return status;
}

- (QStatus)activateInterfacesWithBus:(AJNBusAttachment *)busAttachment
{
    QStatus status;

    status = ER_OK;

    AJNInterfaceDescription *interfaceDescription;

    
        //
        // SecureObjectDelegate interface (org.alljoyn.bus.unittests.secure.SecureInterface)
        //
        // create an interface description, or if that fails, get the interface as it was already created
        //
        interfaceDescription = [busAttachment createInterfaceWithName:@"org.alljoyn.bus.unittests.secure.SecureInterface" withInterfaceSecPolicy:AJN_IFC_SECURITY_REQUIRED];

    
        // add the methods to the interface description
        //
    
        status = [interfaceDescription addMethodWithName:@"concatenate" inputSignature:@"ss" outputSignature:@"s" argumentNames:[NSArray arrayWithObjects:@"str1",@"str2",@"outStr", nil]];
        
        if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
            @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add method to interface: concatenate" userInfo:nil];
        }
    
    

    
        [interfaceDescription activate];


    self.bus = busAttachment;

    return status;
}

- (id)initWithPath:(NSString *)path
{
    self = [super initWithPath:path];
    if (self) {
    // create the internal C++ bus object
    //
        SecureObjectImpl *busObject = new SecureObjectImpl([path UTF8String],(id<SecureObjectDelegate>)self);
        self.handle = busObject;
    }
    return self;
}

- (id)initWithBusAttachment:(AJNBusAttachment *)busAttachment onPath:(NSString *)path
{
    self = [super initWithBusAttachment:busAttachment onPath:path];
    if (self) {
        [self activateInterfacesWithBus:busAttachment];

        // create the internal C++ bus object
        //
        SecureObjectImpl *busObject = new SecureObjectImpl(*((ajn::BusAttachment*)busAttachment.handle), [path UTF8String], (id<SecureObjectDelegate>)self);
        
        self.handle = busObject;
        
      
    }
    return self;
}

- (void)dealloc
{
    SecureObjectImpl *busObject = [self busObject];
    delete busObject;
    self.handle = nil;
}

    
- (NSString*)concatenateString:(NSString*)str1 withString:(NSString*)str2 message:(AJNMessage *)methodCallMessage
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
//  Objective-C Proxy Bus Object implementation for SecureObject
//
////////////////////////////////////////////////////////////////////////////////

@interface SecureObjectProxy(Private)

@property (nonatomic, strong) AJNBusAttachment *bus;

- (ProxyBusObject*)proxyBusObject;

@end

@implementation SecureObjectProxy
    
- (NSString*)concatenateString:(NSString*)str1 withString:(NSString*)str2
{
    [self addInterfaceNamed:@"org.alljoyn.bus.unittests.secure.SecureInterface"];
    
    // prepare the input arguments
    //
    
    Message reply(*((BusAttachment*)self.bus.handle));    
    MsgArg inArgs[2];
    
    inArgs[0].Set("s", [str1 UTF8String]);
        
    inArgs[1].Set("s", [str2 UTF8String]);
        

    // make the function call using the C++ proxy object
    //
    
    QStatus status = self.proxyBusObject->MethodCall("org.alljoyn.bus.unittests.secure.SecureInterface", "concatenate", inArgs, 2, reply, 5000);
    if (ER_OK != status) {
        NSLog(@"ERROR: ProxyBusObject::MethodCall on org.alljoyn.bus.unittests.secure.SecureInterface failed. %@", [AJNStatus descriptionForStatusCode:status]);
        
        return nil;
            
    }

    
    // pass the output arguments back to the caller
    //
    
        
    return [NSString stringWithCString:reply->GetArg()->v_string.str encoding:NSUTF8StringEncoding];
        

}

@end

////////////////////////////////////////////////////////////////////////////////
