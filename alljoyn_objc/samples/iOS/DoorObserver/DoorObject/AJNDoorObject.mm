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
//  AJNDoorObject.mm
//
////////////////////////////////////////////////////////////////////////////////

#import <alljoyn/BusAttachment.h>
#import <alljoyn/BusObject.h>
#import "AJNBusObjectImpl.h"
#import "AJNInterfaceDescription.h"
#import "AJNMessageArgument.h"
#import "AJNSignalHandlerImpl.h"

#import "DoorObject.h"

using namespace ajn;


@interface AJNMessageArgument(Private)

/**
 * Helper to return the C++ API object that is encapsulated by this objective-c class
 */
@property (nonatomic, readonly) MsgArg *msgArg;

@end


////////////////////////////////////////////////////////////////////////////////
//
//  C++ Bus Object class declaration for DoorObjectImpl
//
////////////////////////////////////////////////////////////////////////////////
class DoorObjectImpl : public AJNBusObjectImpl
{
private:
    const InterfaceDescription::Member* PersonPassedThroughSignalMember;


public:
    DoorObjectImpl(BusAttachment &bus, const char *path, id<DoorObjectDelegate> aDelegate);

    // properties
    //
    virtual QStatus Get(const char* ifcName, const char* propName, MsgArg& val);
    virtual QStatus Set(const char* ifcName, const char* propName, MsgArg& val);

    // methods
    //
    void Open(const InterfaceDescription::Member* member, Message& msg);
	void Close(const InterfaceDescription::Member* member, Message& msg);
	void KnockAndRun(const InterfaceDescription::Member* member, Message& msg);

    // signals
    //
    QStatus SendPersonPassedThrough(const char * name, const char* destination, SessionId sessionId, uint16_t timeToLive = 0, uint8_t flags = 0);

};
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  C++ Bus Object implementation for DoorObjectImpl
//
////////////////////////////////////////////////////////////////////////////////

DoorObjectImpl::DoorObjectImpl(BusAttachment &bus, const char *path, id<DoorObjectDelegate> aDelegate) : 
    AJNBusObjectImpl(bus,path,aDelegate)
{
    const InterfaceDescription* interfaceDescription = NULL;
    QStatus status;
    status = ER_OK;

    // Add the com.example.Door interface to this object
    //
    interfaceDescription = bus.GetInterface("com.example.Door");
    assert(interfaceDescription);
    AddInterface(*interfaceDescription, ANNOUNCED);

    
    // Register the method handlers for interface DoorObjectDelegate with the object
    //
    const MethodEntry methodEntriesForDoorObjectDelegate[] = {

        {
            interfaceDescription->GetMember("Open"), static_cast<MessageReceiver::MethodHandler>(&DoorObjectImpl::Open)
        },

        {
            interfaceDescription->GetMember("Close"), static_cast<MessageReceiver::MethodHandler>(&DoorObjectImpl::Close)
        },

        {
            interfaceDescription->GetMember("KnockAndRun"), static_cast<MessageReceiver::MethodHandler>(&DoorObjectImpl::KnockAndRun)
        }
    };

    status = AddMethodHandlers(methodEntriesForDoorObjectDelegate, sizeof(methodEntriesForDoorObjectDelegate) / sizeof(methodEntriesForDoorObjectDelegate[0]));
    if (ER_OK != status) {
        NSLog(@"ERROR: An error occurred while adding method handlers for interface com.example.Door to the interface description. %@", [AJNStatus descriptionForStatusCode:status]);
    }

    // save off signal members for later
    //
    PersonPassedThroughSignalMember = interfaceDescription->GetMember("PersonPassedThrough");
    assert(PersonPassedThroughSignalMember);


}


QStatus DoorObjectImpl::Get(const char* ifcName, const char* propName, MsgArg& val)
{
    QStatus status = ER_BUS_NO_SUCH_PROPERTY;

    @autoreleasepool {

    if (strcmp(ifcName, "com.example.Door") == 0)
    {

        if (strcmp(propName, "IsOpen") == 0)
        {
            status = val.Set( "b", ((id<DoorObjectDelegate>)delegate).IsOpen);
        }

        if (strcmp(propName, "Location") == 0)
        {
            status = val.Set( "s", [((id<DoorObjectDelegate>)delegate).Location UTF8String]);
        }

        if (strcmp(propName, "KeyCode") == 0)
        {
            status = val.Set( "u", [((id<DoorObjectDelegate>)delegate).KeyCode unsignedIntValue]);
        }
    }
    }
    return status;
}

QStatus DoorObjectImpl::Set(const char* ifcName, const char* propName, MsgArg& val)
{
    QStatus status = ER_BUS_NO_SUCH_PROPERTY;

    @autoreleasepool {

    }
    return status;
}

void DoorObjectImpl::Open(const InterfaceDescription::Member *member, Message& msg)
{
    @autoreleasepool {

    // call the Objective-C delegate method
    //

	[(id<DoorObjectDelegate>)delegate open:[[AJNMessage alloc] initWithHandle:&msg]];
    MethodReply(msg);
    }
}

void DoorObjectImpl::Close(const InterfaceDescription::Member *member, Message& msg)
{
    @autoreleasepool {

    // call the Objective-C delegate method
    //

	[(id<DoorObjectDelegate>)delegate close:[[AJNMessage alloc] initWithHandle:&msg]];
    MethodReply(msg);
    }
}

void DoorObjectImpl::KnockAndRun(const InterfaceDescription::Member *member, Message& msg)
{
    @autoreleasepool {

    // call the Objective-C delegate method
    //

	[(id<DoorObjectDelegate>)delegate knockAndRun:[[AJNMessage alloc] initWithHandle:&msg]];
    }
}

QStatus DoorObjectImpl::SendPersonPassedThrough(const char * name, const char* destination, SessionId sessionId, uint16_t timeToLive, uint8_t flags)
{
    MsgArg args[1];
    args[0].Set( "s", name);

    return Signal(destination, sessionId, *PersonPassedThroughSignalMember, args, 1, timeToLive, flags);
}


////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  Objective-C Bus Object implementation for AJNDoorObject
//
////////////////////////////////////////////////////////////////////////////////

@implementation AJNDoorObject

@dynamic handle;

@synthesize IsOpen = _IsOpen;
@synthesize Location = _Location;
@synthesize KeyCode = _KeyCode;


- (DoorObjectImpl*)busObject
{
    return static_cast<DoorObjectImpl*>(self.handle);
}

- (id)initWithBusAttachment:(AJNBusAttachment *)busAttachment onPath:(NSString *)path
{
    self = [super initWithBusAttachment:busAttachment onPath:path];
    if (self) {
        QStatus status;

        status = ER_OK;
        AJNInterfaceDescription *interfaceDescription;

        //
        // DoorObjectDelegate interface (com.example.Door)
        //
        // create an interface description, or if that fails, get the interface as it was already created
        //
        interfaceDescription = [busAttachment createInterfaceWithName:@"com.example.Door"];

        // add the properties to the interface description
        //

        status = [interfaceDescription addPropertyWithName:@"IsOpen" signature:@"b"];
        if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
            @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add property to interface:  IsOpen" userInfo:nil];
        }

        status = [interfaceDescription addPropertyWithName:@"Location" signature:@"s"];
        if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
            @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add property to interface:  Location" userInfo:nil];
        }

        status = [interfaceDescription addPropertyWithName:@"KeyCode" signature:@"u"];
        if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
            @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add property to interface:  KeyCode" userInfo:nil];
        }

        // add the methods to the interface description
        //

        status = [interfaceDescription addMethodWithName:@"Open" inputSignature:@"" outputSignature:@"" argumentNames:[NSArray arrayWithObjects: nil]];
        if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
            @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add method to interface: Open" userInfo:nil];
        }

        status = [interfaceDescription addMethodWithName:@"Close" inputSignature:@"" outputSignature:@"" argumentNames:[NSArray arrayWithObjects: nil]];
        if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
            @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add method to interface: Close" userInfo:nil];
        }

        status = [interfaceDescription addMethodWithName:@"KnockAndRun" inputSignature:@"" outputSignature:@"" argumentNames:[NSArray arrayWithObjects: nil]];
        if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
            @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add method to interface: KnockAndRun" userInfo:nil];
        }

        // add the signals to the interface description
        //

        status = [interfaceDescription addSignalWithName:@"PersonPassedThrough" inputSignature:@"s" argumentNames:[NSArray arrayWithObjects:@"name", nil]];
        if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
            @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add signal to interface:  PersonPassedThrough" userInfo:nil];
        }

        [interfaceDescription activate];

        // create the internal C++ bus object
        //
        DoorObjectImpl *busObject = new DoorObjectImpl(*((ajn::BusAttachment*)busAttachment.handle), [path UTF8String], (id<DoorObjectDelegate>)self);

        self.handle = busObject;
    }
    return self;
}

- (void)dealloc
{
    DoorObjectImpl *busObject = [self busObject];
    delete busObject;
    self.handle = nil;
}

- (void)open:(AJNMessage *)methodCallMessage
{
    //
    // GENERATED CODE - DO NOT EDIT
    //
    // Create a category or subclass in separate .h/.m files
    @throw([NSException exceptionWithName:@"NotImplementedException" reason:@"You must override this method in a subclass" userInfo:nil]);
}

- (void)close:(AJNMessage *)methodCallMessage
{
    //
    // GENERATED CODE - DO NOT EDIT
    //
    // Create a category or subclass in separate .h/.m files
    @throw([NSException exceptionWithName:@"NotImplementedException" reason:@"You must override this method in a subclass" userInfo:nil]);
}

- (void)knockAndRun:(AJNMessage *)methodCallMessage
{
    //
    // GENERATED CODE - DO NOT EDIT
    //
    // Create a category or subclass in separate .h/.m files
    @throw([NSException exceptionWithName:@"NotImplementedException" reason:@"You must override this method in a subclass" userInfo:nil]);
}

- (void)sendPersonPassedThroughName:(NSString*)name inSession:(AJNSessionId)sessionId toDestination:(NSString*)destinationPath

{
    self.busObject->SendPersonPassedThrough([name UTF8String], [destinationPath UTF8String], sessionId);
}


@end

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  Objective-C Proxy Bus Object implementation for DoorObject
//
////////////////////////////////////////////////////////////////////////////////

@interface DoorObjectProxy(Private)

@property (nonatomic, strong) AJNBusAttachment *bus;

- (ProxyBusObject*)proxyBusObject;

@end

@implementation DoorObjectProxy

- (BOOL)IsOpen
{
    [self addInterfaceNamed:@"com.example.Door"];

    MsgArg propValue;
    QStatus status = self.proxyBusObject->GetProperty("com.example.Door", "IsOpen", propValue);

    if (status != ER_OK) {
        NSLog(@"ERROR: Failed to get property IsOpen on interface com.example.Door. %@", [AJNStatus descriptionForStatusCode:status]);
        return NO;
    }
    return propValue.v_variant.val->v_bool;
}

- (NSString*)Location
{
    [self addInterfaceNamed:@"com.example.Door"];

    MsgArg propValue;
    QStatus status = self.proxyBusObject->GetProperty("com.example.Door", "Location", propValue);

    if (status != ER_OK) {
        NSLog(@"ERROR: Failed to get property Location on interface com.example.Door. %@", [AJNStatus descriptionForStatusCode:status]);
        return @"error";
    }
    return [NSString stringWithCString:propValue.v_variant.val->v_string.str encoding:NSUTF8StringEncoding];
}

- (NSNumber*)KeyCode
{
    [self addInterfaceNamed:@"com.example.Door"];

    MsgArg propValue;
    QStatus status = self.proxyBusObject->GetProperty("com.example.Door", "KeyCode", propValue);

    if (status != ER_OK) {
        NSLog(@"ERROR: Failed to get property KeyCode on interface com.example.Door. %@", [AJNStatus descriptionForStatusCode:status]);
        return @0;
    }
    return [NSNumber numberWithUnsignedInt:propValue.v_variant.val->v_uint32];
}

- (void)open
{
    [self addInterfaceNamed:@"com.example.Door"];

    // prepare the input arguments
    //

    Message reply(*((BusAttachment*)self.bus.handle));
    MsgArg inArgs[0];

    // make the function call using the C++ proxy object
    //

    QStatus status = self.proxyBusObject->MethodCall("com.example.Door", "Open", inArgs, 0, reply, 5000);
    if (ER_OK != status) {
        NSLog(@"ERROR: ProxyBusObject::MethodCall on com.example.Door failed. %@", [AJNStatus descriptionForStatusCode:status]);
        return;
    }
}

- (void)close
{
    [self addInterfaceNamed:@"com.example.Door"];

    // prepare the input arguments
    //

    Message reply(*((BusAttachment*)self.bus.handle));
    MsgArg inArgs[0];

    // make the function call using the C++ proxy object
    //

    QStatus status = self.proxyBusObject->MethodCall("com.example.Door", "Close", inArgs, 0, reply, 5000);
    if (ER_OK != status) {
        NSLog(@"ERROR: ProxyBusObject::MethodCall on com.example.Door failed. %@", [AJNStatus descriptionForStatusCode:status]);
        return;
    }
}

- (void)knockAndRun
{
    [self addInterfaceNamed:@"com.example.Door"];

    // prepare the input arguments
    //

    Message reply(*((BusAttachment*)self.bus.handle));
    MsgArg inArgs[0];

    // make the function call using the C++ proxy object
    //

    QStatus status = self.proxyBusObject->MethodCall("com.example.Door", "KnockAndRun", inArgs, 0, reply, 5000);
    if (ER_OK != status) {
        NSLog(@"ERROR: ProxyBusObject::MethodCall on com.example.Door failed. %@", [AJNStatus descriptionForStatusCode:status]);
        return;
    }
}

@end

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  C++ Signal Handler implementation for DoorObjectDelegate
//
////////////////////////////////////////////////////////////////////////////////

class DoorObjectDelegateSignalHandlerImpl : public AJNSignalHandlerImpl
{
private:

    const ajn::InterfaceDescription::Member* PersonPassedThroughSignalMember;
    void PersonPassedThroughSignalHandler(const ajn::InterfaceDescription::Member* member, const char* srcPath, ajn::Message& msg);

public:
    /**
     * Constructor for the AJN signal handler implementation.
     *
     * @param aDelegate         Objective C delegate called when one of the below virtual functions is called.
     */
    DoorObjectDelegateSignalHandlerImpl(id<AJNSignalHandler> aDelegate);

    virtual void RegisterSignalHandler(ajn::BusAttachment &bus);

    virtual void UnregisterSignalHandler(ajn::BusAttachment &bus);

    /**
     * Virtual destructor for derivable class.
     */
    virtual ~DoorObjectDelegateSignalHandlerImpl();
};


/**
 * Constructor for the AJN signal handler implementation.
 *
 * @param aDelegate         Objective C delegate called when one of the below virtual functions is called.     
 */
DoorObjectDelegateSignalHandlerImpl::DoorObjectDelegateSignalHandlerImpl(id<AJNSignalHandler> aDelegate) : AJNSignalHandlerImpl(aDelegate)
{
	PersonPassedThroughSignalMember = NULL;
}

DoorObjectDelegateSignalHandlerImpl::~DoorObjectDelegateSignalHandlerImpl()
{
    m_delegate = NULL;
}

void DoorObjectDelegateSignalHandlerImpl::RegisterSignalHandler(ajn::BusAttachment &bus)
{
    QStatus status;
    status = ER_OK;
    const ajn::InterfaceDescription* interface = NULL;

    ////////////////////////////////////////////////////////////////////////////
    // Register signal handler for signal PersonPassedThrough
    //
    interface = bus.GetInterface("com.example.Door");

    if (interface) {
        // Store the PersonPassedThrough signal member away so it can be quickly looked up
        PersonPassedThroughSignalMember = interface->GetMember("PersonPassedThrough");
        assert(PersonPassedThroughSignalMember);

        // Register signal handler for PersonPassedThrough
        status =  bus.RegisterSignalHandler(this,
            static_cast<MessageReceiver::SignalHandler>(&DoorObjectDelegateSignalHandlerImpl::PersonPassedThroughSignalHandler),
            PersonPassedThroughSignalMember,
            NULL);

        if (status != ER_OK) {
            NSLog(@"ERROR: Interface DoorObjectDelegateSignalHandlerImpl::RegisterSignalHandler failed. %@", [AJNStatus descriptionForStatusCode:status] );
        }
    }
    else {
        NSLog(@"ERROR: com.example.Door not found.");
    }
    ////////////////////////////////////////////////////////////////////////////

}

void DoorObjectDelegateSignalHandlerImpl::UnregisterSignalHandler(ajn::BusAttachment &bus)
{
    QStatus status;
    status = ER_OK;
    const ajn::InterfaceDescription* interface = NULL;

    ////////////////////////////////////////////////////////////////////////////
    // Unregister signal handler for signal PersonPassedThrough
    //
    interface = bus.GetInterface("com.example.Door");

    // Store the PersonPassedThrough signal member away so it can be quickly looked up
    PersonPassedThroughSignalMember = interface->GetMember("PersonPassedThrough");
    assert(PersonPassedThroughSignalMember);

    // Unregister signal handler for PersonPassedThrough
    status =  bus.UnregisterSignalHandler(this,
        static_cast<MessageReceiver::SignalHandler>(&DoorObjectDelegateSignalHandlerImpl::PersonPassedThroughSignalHandler),
        PersonPassedThroughSignalMember,
        NULL);

    if (status != ER_OK) {
        NSLog(@"ERROR:DoorObjectDelegateSignalHandlerImpl::UnregisterSignalHandler failed. %@", [AJNStatus descriptionForStatusCode:status] );
    }
    ////////////////////////////////////////////////////////////////////////////
}


void DoorObjectDelegateSignalHandlerImpl::PersonPassedThroughSignalHandler(const ajn::InterfaceDescription::Member* member, const char* srcPath, ajn::Message& msg)
{
    @autoreleasepool {

    qcc::String inArg0 = msg->GetArg(0)->v_string.str;

        AJNMessage *signalMessage = [[AJNMessage alloc] initWithHandle:&msg];
        NSString *objectPath = [NSString stringWithCString:msg->GetObjectPath() encoding:NSUTF8StringEncoding];
        AJNSessionId sessionId = msg->GetSessionId();
        NSLog(@"Received PersonPassedThrough signal from %@ on path %@ for session id %u [%s > %s]", [signalMessage senderName], objectPath, msg->GetSessionId(), msg->GetRcvEndpointName(), msg->GetDestination() ? msg->GetDestination() : "broadcast");

        dispatch_async(dispatch_get_main_queue(), ^{
            
            [(id<DoorObjectDelegateSignalHandler>)m_delegate didReceivePersonPassedThroughName:[NSString stringWithCString:inArg0.c_str() encoding:NSUTF8StringEncoding] inSession:sessionId message:signalMessage];
        });
    }
}


@implementation AJNBusAttachment(DoorObjectDelegate)

- (void)registerDoorObjectDelegateSignalHandler:(id<DoorObjectDelegateSignalHandler>)signalHandler
{
    DoorObjectDelegateSignalHandlerImpl *signalHandlerImpl = new DoorObjectDelegateSignalHandlerImpl(signalHandler);
    signalHandler.handle = signalHandlerImpl;
    [self registerSignalHandler:signalHandler];
}

@end

////////////////////////////////////////////////////////////////////////////////
