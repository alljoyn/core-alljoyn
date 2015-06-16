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
//  AJNEventsActionObject.mm
//
////////////////////////////////////////////////////////////////////////////////

#import <alljoyn/BusAttachment.h>
#import <alljoyn/BusObject.h>
#import "AJNBusObjectImpl.h"
#import "AJNInterfaceDescription.h"
#import "AJNMessageArgument.h"
#import "AJNSignalHandlerImpl.h"

#import "EventsActionObject.h"

using namespace ajn;


@interface AJNMessageArgument(Private)

/**
 * Helper to return the C++ API object that is encapsulated by this objective-c class
 */
@property (nonatomic, readonly) MsgArg *msgArg;

@end


////////////////////////////////////////////////////////////////////////////////
//
//  C++ Bus Object class declaration for SampleObjectImpl
//
////////////////////////////////////////////////////////////////////////////////
class SampleObjectImpl : public AJNBusObjectImpl
{
private:
    const InterfaceDescription::Member* TestEventSignalMember;

    
public:
    SampleObjectImpl(const char *path, id<SampleObjectDelegate> aDelegate);
    SampleObjectImpl(BusAttachment &bus, const char *path, id<SampleObjectDelegate> aDelegate);

    virtual QStatus AddInterfacesAndHandlers(BusAttachment &bus);

    
    // properties
    //
    virtual QStatus Get(const char* ifcName, const char* propName, MsgArg& val);
    virtual QStatus Set(const char* ifcName, const char* propName, MsgArg& val);        
    
    
    // methods
    //
    void TestAction(const InterfaceDescription::Member* member, Message& msg);

    
    // signals
    //
    QStatus SendTestEvent(const char * outStr, const char* destination, SessionId sessionId, uint16_t timeToLive = 0, uint8_t flags = 0);

};
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  C++ Bus Object implementation for SampleObjectImpl
//
////////////////////////////////////////////////////////////////////////////////

SampleObjectImpl::SampleObjectImpl(const char *path, id<SampleObjectDelegate> aDelegate) :
    AJNBusObjectImpl(path,aDelegate)
{
    // Intentionally empty
}

SampleObjectImpl::SampleObjectImpl(BusAttachment &bus, const char *path, id<SampleObjectDelegate> aDelegate) :
    AJNBusObjectImpl(bus,path,aDelegate)
{
    AddInterfacesAndHandlers(bus);
}

QStatus SampleObjectImpl::AddInterfacesAndHandlers(BusAttachment &bus)
{
    const InterfaceDescription* interfaceDescription = NULL;
    QStatus status;
    status = ER_OK;
    
    
    // Add the org.alljoyn.bus.sample interface to this object
    //
    interfaceDescription = bus.GetInterface("org.alljoyn.bus.sample");
    assert(interfaceDescription);
    status = AddInterface(*interfaceDescription);
    if (ER_OK != status) {
        NSLog(@"ERROR: An error occurred while adding the interface org.alljoyn.bus.sample.%@", [AJNStatus descriptionForStatusCode:status]);
    }
    
    // Register the method handlers for interface SampleObjectDelegate with the object
    //
    const MethodEntry methodEntriesForSampleObjectDelegate[] = {

        {
			interfaceDescription->GetMember("TestAction"), static_cast<MessageReceiver::MethodHandler>(&SampleObjectImpl::TestAction)
		}
    
    };
    
    status = AddMethodHandlers(methodEntriesForSampleObjectDelegate, sizeof(methodEntriesForSampleObjectDelegate) / sizeof(methodEntriesForSampleObjectDelegate[0]));
    if (ER_OK != status) {
        NSLog(@"ERROR: An error occurred while adding method handlers for interface org.alljoyn.bus.sample to the interface description. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    
    // save off signal members for later
    //
    TestEventSignalMember = interfaceDescription->GetMember("TestEvent");
    assert(TestEventSignalMember);    

    return status;
}


QStatus SampleObjectImpl::Get(const char* ifcName, const char* propName, MsgArg& val)
{
    QStatus status = ER_BUS_NO_SUCH_PROPERTY;
    
    @autoreleasepool {
    
    if (strcmp(ifcName, "org.alljoyn.bus.sample") == 0) 
    {
    
        if (strcmp(propName, "TestProperty") == 0)
        {
                
            status = val.Set( "s", [((id<SampleObjectDelegate>)delegate).TestProperty UTF8String] );
            
        }
    
    }
    
    
    }
    
    return status;
}
    
QStatus SampleObjectImpl::Set(const char* ifcName, const char* propName, MsgArg& val)
{
    QStatus status = ER_BUS_NO_SUCH_PROPERTY;
    
    @autoreleasepool {
    
    
    
    }

    return status;
}

void SampleObjectImpl::TestAction(const InterfaceDescription::Member *member, Message& msg)
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
    
	outArg0 = [(id<SampleObjectDelegate>)delegate concatenateString:[NSString stringWithCString:inArg0.c_str() encoding:NSUTF8StringEncoding] withString:[NSString stringWithCString:inArg1.c_str() encoding:NSUTF8StringEncoding] message:[[AJNMessage alloc] initWithHandle:&msg]];
            
        
    // formulate the reply
    //
    MsgArg outArgs[1];
    
    outArgs[0].Set("s", [outArg0 UTF8String]);
        
    QStatus status = MethodReply(msg, outArgs, 1);
    if (ER_OK != status) {
        NSLog(@"ERROR: An error occurred when attempting to send a method reply for TestAction. %@", [AJNStatus descriptionForStatusCode:status]);
    }  
        
    
    }
}

QStatus SampleObjectImpl::SendTestEvent(const char * outStr, const char* destination, SessionId sessionId, uint16_t timeToLive, uint8_t flags)
{

    MsgArg args[1];

    
            args[0].Set( "s", outStr );
        

    return Signal(destination, sessionId, *TestEventSignalMember, args, 1, timeToLive, flags);
}


////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  Objective-C Bus Object implementation for AJNSampleObject
//
////////////////////////////////////////////////////////////////////////////////

@interface AJNSampleObject()
/**
* The bus attachment this object is associated with.
*/
@property (nonatomic, weak) AJNBusAttachment *bus;

@end

@implementation AJNSampleObject

@dynamic handle;
@synthesize bus = _bus;

@synthesize TestProperty = _TestProperty;


- (SampleObjectImpl*)busObject
{
    return static_cast<SampleObjectImpl*>(self.handle);
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
        // SampleObjectDelegate interface (org.alljoyn.bus.sample)
        //
        // create an interface description, or if that fails, get the interface as it was already created
        //
        interfaceDescription = [busAttachment createInterfaceWithName:@"org.alljoyn.bus.sample"];

    
        [interfaceDescription setDescriptionLanguage:@""];
        [interfaceDescription setDescription:@"This is the interface"];
    
        // add the properties to the interface description
        //
    
        status = [interfaceDescription addPropertyWithName:@"TestProperty" signature:@"s"];
        
        if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
            @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add property to interface:  TestProperty" userInfo:nil];
        }
    
        [interfaceDescription setPropertyDescription:@"This is property description" forPropertyWithName:@"TestProperty"];
    
        // add the methods to the interface description
        //
    
        status = [interfaceDescription addMethodWithName:@"TestAction" inputSignature:@"ss" outputSignature:@"s" argumentNames:[NSArray arrayWithObjects:@"str1",@"str2",@"outStr", nil]];
        
        if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
            @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add method to interface: TestAction" userInfo:nil];
        }
    
        [interfaceDescription setMemberDescription:@"This is the test action" forMemberWithName:@"TestAction" sessionlessSignal:FALSE];
    
        // add the signals to the interface description
        //
    
        status = [interfaceDescription addSignalWithName:@"TestEvent" inputSignature:@"s" argumentNames:[NSArray arrayWithObjects:@"outStr", nil]];
        
        if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
            @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add signal to interface:  TestEvent" userInfo:nil];
        }
    
        [interfaceDescription setMemberDescription:@"This is the test event" forMemberWithName:@"TestEvent" sessionlessSignal:FALSE];

    
    

    
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
        SampleObjectImpl *busObject = new SampleObjectImpl([path UTF8String],(id<SampleObjectDelegate>)self);
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
        SampleObjectImpl *busObject = new SampleObjectImpl(*((ajn::BusAttachment*)busAttachment.handle), [path UTF8String], (id<SampleObjectDelegate>)self);
        
        self.handle = busObject;
        
      
    }
    return self;
}

- (void)dealloc
{
    SampleObjectImpl *busObject = [self busObject];
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
- (void)sendtestEventString(NSString*)outStr inSession:(AJNSessionId)sessionId toDestination:(NSString*)destinationPath

{
    
    self.busObject->SendTestEvent([outStr UTF8String], [destinationPath UTF8String], sessionId);
        
}

    
@end

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  Objective-C Proxy Bus Object implementation for SampleObject
//
////////////////////////////////////////////////////////////////////////////////

@interface SampleObjectProxy(Private)

@property (nonatomic, strong) AJNBusAttachment *bus;

- (ProxyBusObject*)proxyBusObject;

@end

@implementation SampleObjectProxy
    
- (NSString*)TestProperty
{
    [self addInterfaceNamed:@"org.alljoyn.bus.sample"];
    
    
    MsgArg propValue;
    
    QStatus status = self.proxyBusObject->GetProperty("org.alljoyn.bus.sample", "TestProperty", propValue);

    if (status != ER_OK) {
        NSLog(@"ERROR: Failed to get property TestProperty on interface org.alljoyn.bus.sample. %@", [AJNStatus descriptionForStatusCode:status]);
        
        return nil;
        
    }

    
    return [NSString stringWithCString:propValue.v_variant.val->v_string.str encoding:NSUTF8StringEncoding];
        
}
    
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
    
    QStatus status = self.proxyBusObject->MethodCall("org.alljoyn.bus.sample", "TestAction", inArgs, 2, reply, 5000);
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

////////////////////////////////////////////////////////////////////////////////
//
//  C++ Signal Handler implementation for SampleObjectDelegate
//
////////////////////////////////////////////////////////////////////////////////

class SampleObjectDelegateSignalHandlerImpl : public AJNSignalHandlerImpl
{
private:

    const ajn::InterfaceDescription::Member* TestEventSignalMember;
    void TestEventSignalHandler(const ajn::InterfaceDescription::Member* member, const char* srcPath, ajn::Message& msg);

    
public:
    /**
     * Constructor for the AJN signal handler implementation.
     *
     * @param aDelegate         Objective C delegate called when one of the below virtual functions is called.     
     */    
    SampleObjectDelegateSignalHandlerImpl(id<AJNSignalHandler> aDelegate);
    
    virtual void RegisterSignalHandler(ajn::BusAttachment &bus);
    
    virtual void UnregisterSignalHandler(ajn::BusAttachment &bus);
    
    /**
     * Virtual destructor for derivable class.
     */
    virtual ~SampleObjectDelegateSignalHandlerImpl();
};


/**
 * Constructor for the AJN signal handler implementation.
 *
 * @param aDelegate         Objective C delegate called when one of the below virtual functions is called.     
 */    
SampleObjectDelegateSignalHandlerImpl::SampleObjectDelegateSignalHandlerImpl(id<AJNSignalHandler> aDelegate) : AJNSignalHandlerImpl(aDelegate)
{
	TestEventSignalMember = NULL;

}

SampleObjectDelegateSignalHandlerImpl::~SampleObjectDelegateSignalHandlerImpl()
{
    m_delegate = NULL;
}

void SampleObjectDelegateSignalHandlerImpl::RegisterSignalHandler(ajn::BusAttachment &bus)
{
    QStatus status;
    status = ER_OK;
    const ajn::InterfaceDescription* interface = NULL;
    
    ////////////////////////////////////////////////////////////////////////////
    // Register signal handler for signal TestEvent
    //
    interface = bus.GetInterface("org.alljoyn.bus.sample");

    if (interface) {
        // Store the TestEvent signal member away so it can be quickly looked up
        TestEventSignalMember = interface->GetMember("TestEvent");
        assert(TestEventSignalMember);

        
        // Register signal handler for TestEvent
        status =  bus.RegisterSignalHandler(this,
            static_cast<MessageReceiver::SignalHandler>(&SampleObjectDelegateSignalHandlerImpl::TestEventSignalHandler),
            TestEventSignalMember,
            NULL);
            
        if (status != ER_OK) {
            NSLog(@"ERROR: Interface SampleObjectDelegateSignalHandlerImpl::RegisterSignalHandler failed. %@", [AJNStatus descriptionForStatusCode:status] );
        }
    }
    else {
        NSLog(@"ERROR: org.alljoyn.bus.sample not found.");
    }
    ////////////////////////////////////////////////////////////////////////////    

}

void SampleObjectDelegateSignalHandlerImpl::UnregisterSignalHandler(ajn::BusAttachment &bus)
{
    QStatus status;
    status = ER_OK;
    const ajn::InterfaceDescription* interface = NULL;
    
    ////////////////////////////////////////////////////////////////////////////
    // Unregister signal handler for signal TestEvent
    //
    interface = bus.GetInterface("org.alljoyn.bus.sample");
    
    // Store the TestEvent signal member away so it can be quickly looked up
    TestEventSignalMember = interface->GetMember("TestEvent");
    assert(TestEventSignalMember);
    
    // Unregister signal handler for TestEvent
    status =  bus.UnregisterSignalHandler(this,
        static_cast<MessageReceiver::SignalHandler>(&SampleObjectDelegateSignalHandlerImpl::TestEventSignalHandler),
        TestEventSignalMember,
        NULL);
        
    if (status != ER_OK) {
        NSLog(@"ERROR:SampleObjectDelegateSignalHandlerImpl::UnregisterSignalHandler failed. %@", [AJNStatus descriptionForStatusCode:status] );
    }
    ////////////////////////////////////////////////////////////////////////////    

}


void SampleObjectDelegateSignalHandlerImpl::TestEventSignalHandler(const ajn::InterfaceDescription::Member* member, const char* srcPath, ajn::Message& msg)
{
    @autoreleasepool {
        
    qcc::String inArg0 = msg->GetArg(0)->v_string.str;
        
        AJNMessage *signalMessage = [[AJNMessage alloc] initWithHandle:&msg];
        NSString *objectPath = [NSString stringWithCString:msg->GetObjectPath() encoding:NSUTF8StringEncoding];
        AJNSessionId sessionId = msg->GetSessionId();        
        NSLog(@"Received TestEvent signal from %@ on path %@ for session id %u [%s > %s]", [signalMessage senderName], objectPath, msg->GetSessionId(), msg->GetRcvEndpointName(), msg->GetDestination() ? msg->GetDestination() : "broadcast");
        
        dispatch_async(dispatch_get_main_queue(), ^{
            
            [(id<SampleObjectDelegateSignalHandler>)m_delegate didReceivetestEventString[NSString stringWithCString:inArg0.c_str() encoding:NSUTF8StringEncoding] inSession:sessionId message:signalMessage];
                
        });
        
    }
}


@implementation AJNBusAttachment(SampleObjectDelegate)

- (void)registerSampleObjectDelegateSignalHandler:(id<SampleObjectDelegateSignalHandler>)signalHandler
{
    SampleObjectDelegateSignalHandlerImpl *signalHandlerImpl = new SampleObjectDelegateSignalHandlerImpl(signalHandler);
    signalHandler.handle = signalHandlerImpl;
    [self registerSignalHandler:signalHandler];
}

@end

////////////////////////////////////////////////////////////////////////////////
    