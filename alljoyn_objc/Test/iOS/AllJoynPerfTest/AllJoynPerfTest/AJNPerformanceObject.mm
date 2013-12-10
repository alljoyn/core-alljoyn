////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013, AllSeen Alliance. All rights reserved.
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
//  AJNPerformanceObject.mm
//
////////////////////////////////////////////////////////////////////////////////

#import <alljoyn/BusAttachment.h>
#import <alljoyn/BusObject.h>
#import "AJNBusObjectImpl.h"
#import "AJNInterfaceDescription.h"
#import "AJNMessageArgument.h"
#import "AJNSignalHandlerImpl.h"

#import "PerformanceObject.h"

using namespace ajn;


@interface AJNMessageArgument(Private)

/**
 * Helper to return the C++ API object that is encapsulated by this objective-c class
 */
@property (nonatomic, readonly) MsgArg *msgArg;

@end


////////////////////////////////////////////////////////////////////////////////
//
//  C++ Bus Object class declaration for PerformanceObjectImpl
//
////////////////////////////////////////////////////////////////////////////////
class PerformanceObjectImpl : public AJNBusObjectImpl
{
private:
    const InterfaceDescription::Member* SendPacketSignalMember;

    
public:
    PerformanceObjectImpl(BusAttachment &bus, const char *path, id<PerformanceObjectDelegate> aDelegate);

    
    
    // methods
    //
    void CheckPacket(const InterfaceDescription::Member* member, Message& msg);

    
    // signals
    //
    QStatus SendSendPacket(int32_t packetIndex,MsgArg* byteArray, const char* destination, SessionId sessionId, uint16_t timeToLive = 0, uint8_t flags = 0);

};
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  C++ Bus Object implementation for PerformanceObjectImpl
//
////////////////////////////////////////////////////////////////////////////////

PerformanceObjectImpl::PerformanceObjectImpl(BusAttachment &bus, const char *path, id<PerformanceObjectDelegate> aDelegate) : 
    AJNBusObjectImpl(bus,path,aDelegate)
{
    const InterfaceDescription* interfaceDescription = NULL;
    QStatus status;
    status = ER_OK;
    
    
    // Add the org.alljoyn.bus.test.perf.both interface to this object
    //
    interfaceDescription = bus.GetInterface("org.alljoyn.bus.test.perf.both");
    assert(interfaceDescription);
    AddInterface(*interfaceDescription);

    
    // Register the method handlers for interface PerformanceObjectDelegate with the object
    //
    const MethodEntry methodEntriesForPerformanceObjectDelegate[] = {

        {
			interfaceDescription->GetMember("CheckPacket"), static_cast<MessageReceiver::MethodHandler>(&PerformanceObjectImpl::CheckPacket)
		}
    
    };
    
    status = AddMethodHandlers(methodEntriesForPerformanceObjectDelegate, sizeof(methodEntriesForPerformanceObjectDelegate) / sizeof(methodEntriesForPerformanceObjectDelegate[0]));
    if (ER_OK != status) {
        NSLog(@"ERROR: An error occurred while adding method handlers for interface org.alljoyn.bus.test.perf.both to the interface description. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    
    // save off signal members for later
    //
    SendPacketSignalMember = interfaceDescription->GetMember("SendPacket");
    assert(SendPacketSignalMember);    


}


void PerformanceObjectImpl::CheckPacket(const InterfaceDescription::Member *member, Message& msg)
{
    @autoreleasepool {
    
    
    
    
    // get all input arguments
    //
    
    int32_t inArg0 = msg->GetArg(0)->v_int32;
        
    AJNMessageArgument* inArg1 = [[AJNMessageArgument alloc] initWithHandle:(AJNHandle)new MsgArg(*(msg->GetArg(1))) shouldDeleteHandleOnDealloc:YES];        
        
    int32_t inArg2 = msg->GetArg(2)->v_int32;
        
    // declare the output arguments
    //
    
	BOOL outArg0;

    
    // call the Objective-C delegate method
    //
    
	outArg0 = [(id<PerformanceObjectDelegate>)delegate checkPacketAtIndex:[NSNumber numberWithInt:inArg0] payLoad:inArg1 packetSize:[NSNumber numberWithInt:inArg2] message:[[AJNMessage alloc] initWithHandle:&msg]];
            
        
    // formulate the reply
    //
    MsgArg outArgs[1];
    
    outArgs[0].Set("b", outArg0 );

    QStatus status = MethodReply(msg, outArgs, 1);
    if (ER_OK != status) {
        NSLog(@"ERROR: An error occurred when attempting to send a method reply for CheckPacket. %@", [AJNStatus descriptionForStatusCode:status]);
    }        
    
    
    }
}


QStatus PerformanceObjectImpl::SendSendPacket(int32_t packetIndex,MsgArg* byteArray, const char* destination, SessionId sessionId, uint16_t timeToLive, uint8_t flags)
{

    MsgArg args[2];

    args[0].Set( "i", packetIndex );
    args[1] = *byteArray;

    return Signal(destination, sessionId, *SendPacketSignalMember, args, 2, timeToLive, flags);
}



////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  Objective-C Bus Object implementation for AJNPerformanceObject
//
////////////////////////////////////////////////////////////////////////////////

@implementation AJNPerformanceObject

@dynamic handle;



- (PerformanceObjectImpl*)busObject
{
    return static_cast<PerformanceObjectImpl*>(self.handle);
}

- (id)initWithBusAttachment:(AJNBusAttachment *)busAttachment onPath:(NSString *)path
{
    self = [super initWithBusAttachment:busAttachment onPath:path];
    if (self) {
        QStatus status;

        status = ER_OK;
        
        AJNInterfaceDescription *interfaceDescription;
        
    
        //
        // PerformanceObjectDelegate interface (org.alljoyn.bus.test.perf.both)
        //
        // create an interface description, or if that fails, get the interface as it was already created
        //
        interfaceDescription = [busAttachment createInterfaceWithName:@"org.alljoyn.bus.test.perf.both"];
        
    
        // add the methods to the interface description
        //
    
        status = [interfaceDescription addMethodWithName:@"CheckPacket" inputSignature:@"iayi" outputSignature:@"b" argumentNames:[NSArray arrayWithObjects:@"packetIndex",@"byteArray",@"packetSize",@"result", nil]];
        
        if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
            @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add method to interface: CheckPacket" userInfo:nil];
        }

        // add the signals to the interface description
        //
    
        status = [interfaceDescription addSignalWithName:@"SendPacket" inputSignature:@"iay" argumentNames:[NSArray arrayWithObjects:@"packetIndex",@"byteArray", nil]];
        
        if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
            @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add signal to interface:  SendPacket" userInfo:nil];
        }

    
        [interfaceDescription activate];


        // create the internal C++ bus object
        //
        PerformanceObjectImpl *busObject = new PerformanceObjectImpl(*((ajn::BusAttachment*)busAttachment.handle), [path UTF8String], (id<PerformanceObjectDelegate>)self);
        
        self.handle = busObject;
    }
    return self;
}

- (void)dealloc
{
    PerformanceObjectImpl *busObject = [self busObject];
    delete busObject;
    self.handle = nil;
}

    
- (BOOL)checkPacketAtIndex:(NSNumber*)packetIndex payLoad:(AJNMessageArgument*)byteArray packetSize:(NSNumber*)packetSize message:(AJNMessage *)methodCallMessage
{
    //
    // GENERATED CODE - DO NOT EDIT
    //
    // Create a category or subclass in separate .h/.m files
    @throw([NSException exceptionWithName:@"NotImplementedException" reason:@"You must override this method in a subclass" userInfo:nil]);
}
- (void)sendPacketAtIndex:(NSNumber*)packetIndex payLoad:(AJNMessageArgument*)byteArray inSession:(AJNSessionId)sessionId toDestination:(NSString*)destinationPath flags:(AJNMessageFlag)flags

{
    
    self.busObject->SendSendPacket([packetIndex intValue], [byteArray msgArg], [destinationPath UTF8String], sessionId, 0, flags);
        
}

    
@end

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  Objective-C Proxy Bus Object implementation for PerformanceObject
//
////////////////////////////////////////////////////////////////////////////////

@interface PerformanceObjectProxy(Private)

@property (nonatomic, strong) AJNBusAttachment *bus;

- (ProxyBusObject*)proxyBusObject;

@end

@implementation PerformanceObjectProxy
    
- (BOOL)checkPacketAtIndex:(NSNumber*)packetIndex payLoad:(AJNMessageArgument*)byteArray packetSize:(NSNumber*)packetSize
{
    [self addInterfaceNamed:@"org.alljoyn.bus.test.perf.both"];
    
    // prepare the input arguments
    //
    
    Message reply(*((BusAttachment*)self.bus.handle));    
    MsgArg inArgs[3];
    
    inArgs[0].Set("i", [packetIndex intValue]);

    inArgs[1] = *[byteArray msgArg];

    inArgs[2].Set("i", [packetSize intValue]);


    // make the function call using the C++ proxy object
    //
    
    QStatus status = self.proxyBusObject->MethodCall("org.alljoyn.bus.test.perf.both", "CheckPacket", inArgs, 3, reply, 5000);
    if (ER_OK != status) {
        NSLog(@"ERROR: ProxyBusObject::MethodCall on org.alljoyn.bus.test.perf.both failed. %@", [AJNStatus descriptionForStatusCode:status]);
        
        return nil;
            
    }

    
    // pass the output arguments back to the caller
    //
    
        
    return reply->GetArg()->v_bool;
        

}

@end

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  C++ Signal Handler implementation for PerformanceObjectDelegate
//
////////////////////////////////////////////////////////////////////////////////

class PerformanceObjectDelegateSignalHandlerImpl : public AJNSignalHandlerImpl
{
private:

    const ajn::InterfaceDescription::Member* SendPacketSignalMember;
    void SendPacketSignalHandler(const ajn::InterfaceDescription::Member* member, const char* srcPath, ajn::Message& msg);

    
public:
    /**
     * Constructor for the AJN signal handler implementation.
     *
     * @param aDelegate         Objective C delegate called when one of the below virtual functions is called.     
     */    
    PerformanceObjectDelegateSignalHandlerImpl(id<AJNSignalHandler> aDelegate);
    
    virtual void RegisterSignalHandler(ajn::BusAttachment &bus);
    
    virtual void UnregisterSignalHandler(ajn::BusAttachment &bus);
    
    /**
     * Virtual destructor for derivable class.
     */
    virtual ~PerformanceObjectDelegateSignalHandlerImpl();
};


/**
 * Constructor for the AJN signal handler implementation.
 *
 * @param aDelegate         Objective C delegate called when one of the below virtual functions is called.     
 */    
PerformanceObjectDelegateSignalHandlerImpl::PerformanceObjectDelegateSignalHandlerImpl(id<AJNSignalHandler> aDelegate) : AJNSignalHandlerImpl(aDelegate)
{
	SendPacketSignalMember = NULL;

}

PerformanceObjectDelegateSignalHandlerImpl::~PerformanceObjectDelegateSignalHandlerImpl()
{
    m_delegate = NULL;
}

void PerformanceObjectDelegateSignalHandlerImpl::RegisterSignalHandler(ajn::BusAttachment &bus)
{
    QStatus status;
    status = ER_OK;
    const ajn::InterfaceDescription* interface = NULL;
    
    ////////////////////////////////////////////////////////////////////////////
    // Register signal handler for signal SendPacket
    //
    interface = bus.GetInterface("org.alljoyn.bus.test.perf.both");

    if (interface) {
        // Store the SendPacket signal member away so it can be quickly looked up
        SendPacketSignalMember = interface->GetMember("SendPacket");
        assert(SendPacketSignalMember);

        
        // Register signal handler for SendPacket
        status =  bus.RegisterSignalHandler(this,
            static_cast<MessageReceiver::SignalHandler>(&PerformanceObjectDelegateSignalHandlerImpl::SendPacketSignalHandler),
            SendPacketSignalMember,
            NULL);
            
        if (status != ER_OK) {
            NSLog(@"ERROR: Interface PerformanceObjectDelegateSignalHandlerImpl::RegisterSignalHandler failed. %@", [AJNStatus descriptionForStatusCode:status] );
        }
    }
    else {
        NSLog(@"ERROR: org.alljoyn.bus.test.perf.both not found.");
    }
    ////////////////////////////////////////////////////////////////////////////    

}

void PerformanceObjectDelegateSignalHandlerImpl::UnregisterSignalHandler(ajn::BusAttachment &bus)
{
    QStatus status;
    status = ER_OK;
    const ajn::InterfaceDescription* interface = NULL;
    
    ////////////////////////////////////////////////////////////////////////////
    // Unregister signal handler for signal SendPacket
    //
    interface = bus.GetInterface("org.alljoyn.bus.test.perf.both");
    
    // Store the SendPacket signal member away so it can be quickly looked up
    SendPacketSignalMember = interface->GetMember("SendPacket");
    assert(SendPacketSignalMember);
    
    // Unregister signal handler for SendPacket
    status =  bus.UnregisterSignalHandler(this,
        static_cast<MessageReceiver::SignalHandler>(&PerformanceObjectDelegateSignalHandlerImpl::SendPacketSignalHandler),
        SendPacketSignalMember,
        NULL);
        
    if (status != ER_OK) {
        NSLog(@"ERROR:PerformanceObjectDelegateSignalHandlerImpl::UnregisterSignalHandler failed. %@", [AJNStatus descriptionForStatusCode:status] );
    }
    ////////////////////////////////////////////////////////////////////////////    

}


void PerformanceObjectDelegateSignalHandlerImpl::SendPacketSignalHandler(const ajn::InterfaceDescription::Member* member, const char* srcPath, ajn::Message& msg)
{
    @autoreleasepool {
        
    int32_t inArg0 = msg->GetArg(0)->v_int32;
        
    AJNMessageArgument* inArg1 = [[AJNMessageArgument alloc] initWithHandle:(AJNHandle)new MsgArg(*(msg->GetArg(1))) shouldDeleteHandleOnDealloc:YES];        
        
        AJNMessage *signalMessage = [[AJNMessage alloc] initWithHandle:&msg];
//        NSString *objectPath = [NSString stringWithCString:msg->GetObjectPath() encoding:NSUTF8StringEncoding];
        AJNSessionId sessionId = msg->GetSessionId();        
//        NSLog(@"Received SendPacket signal from %@ on path %@ for session id %u [%s > %s]", [signalMessage senderName], objectPath, msg->GetSessionId(), msg->GetRcvEndpointName(), msg->GetDestination() ? msg->GetDestination() : "broadcast");
        
        dispatch_async(dispatch_get_main_queue(), ^{
            
            [(id<PerformanceObjectDelegateSignalHandler>)m_delegate didReceivePacketAtIndex:[NSNumber numberWithInt:inArg0] payLoad:inArg1 inSession:sessionId message:signalMessage];
                
        });
        
    }
}


@implementation AJNBusAttachment(PerformanceObjectDelegate)

- (void)registerPerformanceObjectDelegateSignalHandler:(id<PerformanceObjectDelegateSignalHandler>)signalHandler
{
    PerformanceObjectDelegateSignalHandlerImpl *signalHandlerImpl = new PerformanceObjectDelegateSignalHandlerImpl(signalHandler);
    signalHandler.handle = signalHandlerImpl;
    [self registerSignalHandler:signalHandler];
}

@end

////////////////////////////////////////////////////////////////////////////////
    