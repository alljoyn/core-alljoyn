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
//  AJNSecureDoor.mm
//
////////////////////////////////////////////////////////////////////////////////

#import <alljoyn/BusAttachment.h>
#import <alljoyn/BusObject.h>
#import "AJNBusObjectImpl.h"
#import "AJNInterfaceDescription.h"
#import "AJNMessageArgument.h"
#import "AJNSignalHandlerImpl.h"

#import "SecureDoor.h"

using namespace ajn;

@interface AJNMessageArgument(Private)

/**
 * Helper to return the C++ API object that is encapsulated by this objective-c class
 */
@property (nonatomic, readonly) MsgArg *msgArg;

@end


////////////////////////////////////////////////////////////////////////////////
//
//  C++ Bus Object class declaration for DoorImpl
//
////////////////////////////////////////////////////////////////////////////////
class DoorImpl : public AJNBusObjectImpl
{
private:
    const InterfaceDescription::Member* StateChangedSignalMember;


public:
    DoorImpl(const char *path, id<DoorDelegate> aDelegate);
    DoorImpl(BusAttachment &bus, const char *path, id<DoorDelegate> aDelegate);

    virtual QStatus AddInterfacesAndHandlers(BusAttachment &bus);


    // properties
    //
    virtual QStatus Get(const char* ifcName, const char* propName, MsgArg& val);
    virtual QStatus Set(const char* ifcName, const char* propName, MsgArg& val);


    // methods
    //
    void Open(const InterfaceDescription::Member* member, Message& msg);
	void Close(const InterfaceDescription::Member* member, Message& msg);
	void GetState(const InterfaceDescription::Member* member, Message& msg);


    // signals
    //
    QStatus SendStateChanged(bool State, const char* destination, SessionId sessionId, uint16_t timeToLive = 0, uint8_t flags = 0);

};
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  C++ Bus Object implementation for DoorImpl
//
////////////////////////////////////////////////////////////////////////////////

DoorImpl::DoorImpl(const char *path, id<DoorDelegate> aDelegate) :
    AJNBusObjectImpl(path,aDelegate)
{
    // Intentionally empty
}

DoorImpl::DoorImpl(BusAttachment &bus, const char *path, id<DoorDelegate> aDelegate) :
    AJNBusObjectImpl(bus,path,aDelegate)
{
    AddInterfacesAndHandlers(bus);
}

QStatus DoorImpl::AddInterfacesAndHandlers(BusAttachment &bus)
{
    const InterfaceDescription* interfaceDescription = NULL;
    QStatus status = ER_OK;


    // Add the sample.securitymgr.door.Door interface to this object
    //
    interfaceDescription = bus.GetInterface("sample.securitymgr.door.Door");
    assert(interfaceDescription);
    status = AddInterface(*interfaceDescription , ANNOUNCED);
    if (ER_OK != status) {
        NSLog(@"ERROR: An error occurred while adding the interface sample.securitymgr.door.Door.%@", [AJNStatus descriptionForStatusCode:status]);
    }

    // Register the method handlers for interface DoorDelegate with the object
    //
    const MethodEntry methodEntriesForDoorDelegate[] = {

        {
			interfaceDescription->GetMember("Open"), static_cast<MessageReceiver::MethodHandler>(&DoorImpl::Open)
		},

		{
			interfaceDescription->GetMember("Close"), static_cast<MessageReceiver::MethodHandler>(&DoorImpl::Close)
		},

		{
			interfaceDescription->GetMember("GetState"), static_cast<MessageReceiver::MethodHandler>(&DoorImpl::GetState)
		}

    };

    status = AddMethodHandlers(methodEntriesForDoorDelegate, sizeof(methodEntriesForDoorDelegate) / sizeof(methodEntriesForDoorDelegate[0]));
    if (ER_OK != status) {
        NSLog(@"ERROR: An error occurred while adding method handlers for interface sample.securitymgr.door.Door to the interface description. %@", [AJNStatus descriptionForStatusCode:status]);
    }

    // save off signal members for later
    //
    StateChangedSignalMember = interfaceDescription->GetMember("StateChanged");
    assert(StateChangedSignalMember);


    return status;
}


QStatus DoorImpl::Get(const char* ifcName, const char* propName, MsgArg& val)
{
    QStatus status = ER_BUS_NO_SUCH_PROPERTY;

    @autoreleasepool {

    if (strcmp(ifcName, "sample.securitymgr.door.Door") == 0)
    {

        if (strcmp(propName, "State") == 0)
        {

            status = val.Set( "b", ((id<DoorDelegate>)delegate).State  );

        }

    }


    } //autoreleasepool

    return status;
}

QStatus DoorImpl::Set(const char* ifcName, const char* propName, MsgArg& val)
{
    QStatus status = ER_BUS_NO_SUCH_PROPERTY;

    @autoreleasepool {



    } //autoreleasepool

    return status;
}

void DoorImpl::Open(const InterfaceDescription::Member *member, Message& msg)
{
    @autoreleasepool {


    // declare the output arguments
    //

	BOOL outArg0;


    // call the Objective-C delegate method
    //

	[(id<DoorDelegate>)delegate open:&outArg0  message:[[AJNMessage alloc] initWithHandle:&msg]];


    // formulate the reply
    //
    MsgArg outArgs[1];

    outArgs[0].Set("b", outArg0 );

    QStatus status = MethodReply(msg, outArgs, 1);
    if (ER_OK != status) {
        NSLog(@"ERROR: An error occurred when attempting to send a method reply for Open. %@", [AJNStatus descriptionForStatusCode:status]);
    }


    } //autoreleasepool
}

void DoorImpl::Close(const InterfaceDescription::Member *member, Message& msg)
{
    @autoreleasepool {


    // declare the output arguments
    //

	BOOL outArg0;


    // call the Objective-C delegate method
    //

	[(id<DoorDelegate>)delegate close:&outArg0  message:[[AJNMessage alloc] initWithHandle:&msg]];


    // formulate the reply
    //
    MsgArg outArgs[1];

    outArgs[0].Set("b", outArg0 );

    QStatus status = MethodReply(msg, outArgs, 1);
    if (ER_OK != status) {
        NSLog(@"ERROR: An error occurred when attempting to send a method reply for Close. %@", [AJNStatus descriptionForStatusCode:status]);
    }


    } //autoreleasepool
}

void DoorImpl::GetState(const InterfaceDescription::Member *member, Message& msg)
{
    @autoreleasepool {


    // declare the output arguments
    //

	BOOL outArg0;


    // call the Objective-C delegate method
    //

	[(id<DoorDelegate>)delegate getStateMethod:&outArg0  message:[[AJNMessage alloc] initWithHandle:&msg]];


    // formulate the reply
    //
    MsgArg outArgs[1];

    outArgs[0].Set("b", outArg0 );

    QStatus status = MethodReply(msg, outArgs, 1);
    if (ER_OK != status) {
        NSLog(@"ERROR: An error occurred when attempting to send a method reply for GetState. %@", [AJNStatus descriptionForStatusCode:status]);
    }


    } //autoreleasepool
}

QStatus DoorImpl::SendStateChanged(bool State, const char* destination, SessionId sessionId, uint16_t timeToLive, uint8_t flags)
{

    MsgArg args[1];


            args[0].Set( "b", State );


    return Signal(destination, sessionId, *StateChangedSignalMember, args, 1, timeToLive, flags);
}


////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  Objective-C Bus Object implementation for AJNDoor
//
////////////////////////////////////////////////////////////////////////////////

@interface AJNDoor()
/**
* The bus attachment this object is associated with.
*/
@property (nonatomic, weak) AJNBusAttachment *bus;

@end

@implementation AJNDoor

@dynamic handle;
@synthesize bus = _bus;

@synthesize State = _State;


- (DoorImpl*)busObject
{
    return static_cast<DoorImpl*>(self.handle);
}

- (QStatus)registerInterfacesWithBus:(AJNBusAttachment *)busAttachment
{
    QStatus status = [self activateInterfacesWithBus: busAttachment];

    self.busObject->AddInterfacesAndHandlers(*((ajn::BusAttachment*)busAttachment.handle));

    return status;
}

- (QStatus)activateInterfacesWithBus:(AJNBusAttachment *)busAttachment
{
    QStatus status = ER_OK;

    AJNInterfaceDescription *interfaceDescription;


    //
    // DoorDelegate interface (sample.securitymgr.door.Door)
    //
    // create an interface description, or if that fails, get the interface as it was already created
    //
    interfaceDescription = [busAttachment createInterfaceWithName:@"sample.securitymgr.door.Door" enableSecurity:YES];


    // add the properties to the interface description
    //

    status = [interfaceDescription addPropertyWithName:@"State" signature:@"b" accessPermissions:kAJNInterfacePropertyAccessReadFlag];

    if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
        @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add property to interface: State" userInfo:nil];
    }

    // add the methods to the interface description
    //

    status = [interfaceDescription addMethodWithName:@"Open" inputSignature:@"" outputSignature:@"b" argumentNames:[NSArray arrayWithObjects:@"success", nil]];

    if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
        @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add method to interface: Open" userInfo:nil];
    }

    status = [interfaceDescription addMethodWithName:@"Close" inputSignature:@"" outputSignature:@"b" argumentNames:[NSArray arrayWithObjects:@"success", nil]];

    if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
        @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add method to interface: Close" userInfo:nil];
    }

    status = [interfaceDescription addMethodWithName:@"GetState" inputSignature:@"" outputSignature:@"b" argumentNames:[NSArray arrayWithObjects:@"state", nil]];

    if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
        @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add method to interface: GetState" userInfo:nil];
    }

    // add the signals to the interface description
    //

    status = [interfaceDescription addSignalWithName:@"StateChanged" inputSignature:@"b" argumentNames:[NSArray arrayWithObjects:@"State", nil]];

    if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
        @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add signal to interface:  StateChanged" userInfo:nil];
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
        DoorImpl *busObject = new DoorImpl([path UTF8String],(id<DoorDelegate>)self);
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
        DoorImpl *busObject = new DoorImpl(*((ajn::BusAttachment*)busAttachment.handle), [path UTF8String], (id<DoorDelegate>)self);

        self.handle = busObject;


    }
    return self;
}

- (void)dealloc
{
    DoorImpl *busObject = [self busObject];
    delete busObject;
    self.handle = nil;
}


- (QStatus) open:(BOOL*)success message:(AJNMessage *)methodCallMessage;

{
    //
    // GENERATED CODE - DO NOT EDIT
    //
    // Create a category or subclass in separate .h/.m files
    @throw([NSException exceptionWithName:@"NotImplementedException" reason:@"You must override this method in a subclass" userInfo:nil]);
}

- (QStatus) close:(BOOL*)success message:(AJNMessage *)methodCallMessage;

{
    //
    // GENERATED CODE - DO NOT EDIT
    //
    // Create a category or subclass in separate .h/.m files
    @throw([NSException exceptionWithName:@"NotImplementedException" reason:@"You must override this method in a subclass" userInfo:nil]);
}

- (QStatus) getStateMethod:(BOOL*)state message:(AJNMessage *)methodCallMessage;

{
    //
    // GENERATED CODE - DO NOT EDIT
    //
    // Create a category or subclass in separate .h/.m files
    @throw([NSException exceptionWithName:@"NotImplementedException" reason:@"You must override this method in a subclass" userInfo:nil]);
}
- (QStatus)sendstate:(BOOL)State inSession:(AJNSessionId)sessionId toDestination:(NSString *)destinationPath

{

    return self.busObject->SendStateChanged((State == YES ? true : false), [destinationPath UTF8String], sessionId);

}


@end

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  Objective-C Proxy Bus Object implementation for Door
//
////////////////////////////////////////////////////////////////////////////////

@interface DoorProxy(Private)

@property (nonatomic, strong) AJNBusAttachment *bus;

- (ProxyBusObject*)proxyBusObject;

@end

@implementation DoorProxy

- (QStatus)getState:(BOOL*)prop
{
    [self addInterfaceNamed:@"sample.securitymgr.door.Door"];


    MsgArg propValue;

    QStatus status = self.proxyBusObject->GetProperty("sample.securitymgr.door.Door", "State", propValue);

    if (status != ER_OK) {
        NSLog(@"ERROR: Failed to get property State on interface sample.securitymgr.door.Door. %@", [AJNStatus descriptionForStatusCode:status]);

        *prop = NO;

    } else {

    *prop = propValue.v_variant.val->v_bool;

    }
    return status;

}

- (QStatus) open:(BOOL*)success

{
    QStatus status;
    status = [self  open:success replyMessage:nil];
    return status;
}

- (QStatus) open:(BOOL*)success replyMessage:(AJNMessage **)replyMessage

{
    [self addInterfaceNamed:@"sample.securitymgr.door.Door"];

    // prepare the input arguments
    //

    Message reply(*((BusAttachment*)self.bus.handle));
    MsgArg inArgs[0];


    // make the function call using the C++ proxy object
    //

    QStatus status = self.proxyBusObject->MethodCall("sample.securitymgr.door.Door", "Open", inArgs, 0, reply, 5000);
    if (ER_OK != status) {
        NSLog(@"ERROR: ProxyBusObject::MethodCall on sample.securitymgr.door.Door failed. %@", [AJNStatus descriptionForStatusCode:status]);

        // pass nil to the output arguments back after fail
        //


    } else {

        // pass the output arguments back to the caller
        //

        *success = reply->GetArg(0)->v_bool;

    }

    if (replyMessage != nil) {
        *replyMessage = [[AJNMessage alloc] initWithHandle:new Message(reply)];
        }
    return status;
}

- (QStatus) close:(BOOL*)success

{
    QStatus status;
    status = [self  close:success replyMessage:nil];
    return status;
}

- (QStatus) close:(BOOL*)success replyMessage:(AJNMessage **)replyMessage

{
    [self addInterfaceNamed:@"sample.securitymgr.door.Door"];

    // prepare the input arguments
    //

    Message reply(*((BusAttachment*)self.bus.handle));
    MsgArg inArgs[0];


    // make the function call using the C++ proxy object
    //

    QStatus status = self.proxyBusObject->MethodCall("sample.securitymgr.door.Door", "Close", inArgs, 0, reply, 5000);
    if (ER_OK != status) {
        NSLog(@"ERROR: ProxyBusObject::MethodCall on sample.securitymgr.door.Door failed. %@", [AJNStatus descriptionForStatusCode:status]);

        // pass nil to the output arguments back after fail
        //


    } else {

        // pass the output arguments back to the caller
        //

        *success = reply->GetArg(0)->v_bool;

    }

    if (replyMessage != nil) {
        *replyMessage = [[AJNMessage alloc] initWithHandle:new Message(reply)];
        }
    return status;
}

- (QStatus) getStateMethod:(BOOL*)state

{
    QStatus status;
    status = [self  getStateMethod:state replyMessage:nil];
    return status;
}

- (QStatus) getStateProperty:(BOOL*)state

{
    QStatus status;
    status = [self  getStateMethod:state replyMessage:nil];
    return status;
}

- (QStatus) getStateMethod:(BOOL*)state replyMessage:(AJNMessage **)replyMessage

{
    [self addInterfaceNamed:@"sample.securitymgr.door.Door"];

    // prepare the input arguments
    //

    Message reply(*((BusAttachment*)self.bus.handle));
    MsgArg inArgs[0];


    // make the function call using the C++ proxy object
    //

    QStatus status = self.proxyBusObject->MethodCall("sample.securitymgr.door.Door", "GetState", inArgs, 0, reply, 5000);
    if (ER_OK != status) {
        NSLog(@"ERROR: ProxyBusObject::MethodCall on sample.securitymgr.door.Door failed. %@", [AJNStatus descriptionForStatusCode:status]);

        // pass nil to the output arguments back after fail
        //


    } else {

        // pass the output arguments back to the caller
        //

        *state = reply->GetArg(0)->v_bool;

    }

    if (replyMessage != nil) {
        *replyMessage = [[AJNMessage alloc] initWithHandle:new Message(reply)];
        }
    return status;
}

@end

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  C++ Signal Handler implementation for DoorDelegate
//
////////////////////////////////////////////////////////////////////////////////

class DoorDelegateSignalHandlerImpl : public AJNSignalHandlerImpl
{
private:

    const ajn::InterfaceDescription::Member* StateChangedSignalMember;
    void StateChangedSignalHandler(const ajn::InterfaceDescription::Member* member, const char* srcPath, ajn::Message& msg);


public:
    /**
     * Constructor for the AJN signal handler implementation.
     *
     * @param aDelegate         Objective C delegate called when one of the below virtual functions is called.
     */
    DoorDelegateSignalHandlerImpl(id<AJNSignalHandler> aDelegate);

    virtual void RegisterSignalHandler(ajn::BusAttachment &bus);

    virtual void UnregisterSignalHandler(ajn::BusAttachment &bus);

    /**
     * Virtual destructor for derivable class.
     */
    virtual ~DoorDelegateSignalHandlerImpl();
};


/**
 * Constructor for the AJN signal handler implementation.
 *
 * @param aDelegate         Objective C delegate called when one of the below virtual functions is called.
 */
DoorDelegateSignalHandlerImpl::DoorDelegateSignalHandlerImpl(id<AJNSignalHandler> aDelegate) : AJNSignalHandlerImpl(aDelegate)
{
	StateChangedSignalMember = NULL;

}

DoorDelegateSignalHandlerImpl::~DoorDelegateSignalHandlerImpl()
{
    m_delegate = NULL;
}

void DoorDelegateSignalHandlerImpl::RegisterSignalHandler(ajn::BusAttachment &bus)
{
    QStatus status = ER_OK;
    const ajn::InterfaceDescription* interface = NULL;

    ////////////////////////////////////////////////////////////////////////////
    // Register signal handler for signal StateChanged
    //
    interface = bus.GetInterface("sample.securitymgr.door.Door");

    if (interface) {
        // Store the StateChanged signal member away so it can be quickly looked up
        StateChangedSignalMember = interface->GetMember("StateChanged");
        assert(StateChangedSignalMember);

        // Register signal handler for StateChanged
        status =  bus.RegisterSignalHandler(this,
            static_cast<MessageReceiver::SignalHandler>(&DoorDelegateSignalHandlerImpl::StateChangedSignalHandler),
            StateChangedSignalMember,
            NULL);

        if (status != ER_OK) {
            NSLog(@"ERROR: Interface DoorDelegateSignalHandlerImpl::RegisterSignalHandler failed. %@", [AJNStatus descriptionForStatusCode:status] );
        }
    }
    else {
        NSLog(@"ERROR: sample.securitymgr.door.Door not found.");
    }
    ////////////////////////////////////////////////////////////////////////////

}

void DoorDelegateSignalHandlerImpl::UnregisterSignalHandler(ajn::BusAttachment &bus)
{
    QStatus status = ER_OK;
    const ajn::InterfaceDescription* interface = NULL;

    ////////////////////////////////////////////////////////////////////////////
    // Unregister signal handler for signal StateChanged
    //
    interface = bus.GetInterface("sample.securitymgr.door.Door");

    // Store the StateChanged signal member away so it can be quickly looked up
    StateChangedSignalMember = interface->GetMember("StateChanged");
    assert(StateChangedSignalMember);

    // Unregister signal handler for StateChanged
    status =  bus.UnregisterSignalHandler(this,
        static_cast<MessageReceiver::SignalHandler>(&DoorDelegateSignalHandlerImpl::StateChangedSignalHandler),
        StateChangedSignalMember,
        NULL);

    if (status != ER_OK) {
        NSLog(@"ERROR:DoorDelegateSignalHandlerImpl::UnregisterSignalHandler failed. %@", [AJNStatus descriptionForStatusCode:status] );
    }
    ////////////////////////////////////////////////////////////////////////////

}


void DoorDelegateSignalHandlerImpl::StateChangedSignalHandler(const ajn::InterfaceDescription::Member* member, const char* srcPath, ajn::Message& msg)
{
    @autoreleasepool {

    bool inArg0 = msg->GetArg(0)->v_bool;

        AJNMessage *signalMessage = [[AJNMessage alloc] initWithHandle:&msg];
        NSString *objectPath = [NSString stringWithCString:msg->GetObjectPath() encoding:NSUTF8StringEncoding];
        AJNSessionId sessionId = msg->GetSessionId();
        NSLog(@"Received StateChanged signal from %@ on path %@ for session id %u [%s > %s]", [signalMessage senderName], objectPath, msg->GetSessionId(), msg->GetRcvEndpointName(), msg->GetDestination() ? msg->GetDestination() : "broadcast");

        dispatch_async(dispatch_get_main_queue(), ^{

            [(id<DoorDelegateSignalHandler>)m_delegate didReceivestate:(inArg0 ? YES : NO) inSession:sessionId message:signalMessage];

        });
    }
}


@implementation AJNBusAttachment(DoorDelegate)

- (void)registerDoorDelegateSignalHandler:(id<DoorDelegateSignalHandler>)signalHandler
{
    DoorDelegateSignalHandlerImpl *signalHandlerImpl = new DoorDelegateSignalHandlerImpl(signalHandler);
    signalHandler.handle = signalHandlerImpl;
    [self registerSignalHandler:signalHandler];
}

@end

////////////////////////////////////////////////////////////////////////////////

