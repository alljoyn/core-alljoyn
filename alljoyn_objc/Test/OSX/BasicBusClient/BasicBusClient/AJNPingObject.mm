////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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
//  AJNPingObject.mm
//
////////////////////////////////////////////////////////////////////////////////

#import <alljoyn/BusAttachment.h>
#import <alljoyn/BusObject.h>
#import "AJNBusObjectImpl.h"
#import "AJNInterfaceDescription.h"
#import "AJNMessageArgument.h"
#import "AJNSignalHandlerImpl.h"

#import "PingObject.h"

using namespace ajn;

////////////////////////////////////////////////////////////////////////////////
//
//  C++ Bus Object class declaration for PingObjectImpl
//
////////////////////////////////////////////////////////////////////////////////
class PingObjectImpl : public AJNBusObjectImpl
{
private:
    const InterfaceDescription::Member* my_signalSignalMember;

    
public:
    PingObjectImpl(BusAttachment &bus, const char *path, id<PingObjectDelegate, PingObjectValuesDelegate> aDelegate);

    
    // properties
    //
    virtual QStatus Get(const char* ifcName, const char* propName, MsgArg& val);
    virtual QStatus Set(const char* ifcName, const char* propName, MsgArg& val);        
    
    
    // methods
    //
    void delayed_ping(const InterfaceDescription::Member* member, Message& msg);
	void my_ping(const InterfaceDescription::Member* member, Message& msg);
	void time_ping(const InterfaceDescription::Member* member, Message& msg);

    
    // signals
    //
    QStatus Sendmy_signal( const char* destination, SessionId sessionId, uint16_t timeToLive = 0, uint8_t flags = 0);

};
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  C++ Bus Object implementation for PingObjectImpl
//
////////////////////////////////////////////////////////////////////////////////

PingObjectImpl::PingObjectImpl(BusAttachment &bus, const char *path, id<PingObjectDelegate, PingObjectValuesDelegate> aDelegate) : 
    AJNBusObjectImpl(bus,path,aDelegate)
{
    const InterfaceDescription* interfaceDescription = NULL;
    QStatus status;
    status = ER_OK;
    
    
    // Add the org.alljoyn.alljoyn_test interface to this object
    //
    interfaceDescription = bus.GetInterface("org.alljoyn.alljoyn_test");
    assert(interfaceDescription);
    AddInterface(*interfaceDescription);

    
    // Register the method handlers for interface PingObjectDelegate with the object
    //
    const MethodEntry methodEntriesForPingObjectDelegate[] = {

        {
			interfaceDescription->GetMember("delayed_ping"), static_cast<MessageReceiver::MethodHandler>(&PingObjectImpl::delayed_ping)
		},

		{
			interfaceDescription->GetMember("my_ping"), static_cast<MessageReceiver::MethodHandler>(&PingObjectImpl::my_ping)
		},

		{
			interfaceDescription->GetMember("time_ping"), static_cast<MessageReceiver::MethodHandler>(&PingObjectImpl::time_ping)
		}
    
    };
    
    status = AddMethodHandlers(methodEntriesForPingObjectDelegate, sizeof(methodEntriesForPingObjectDelegate) / sizeof(methodEntriesForPingObjectDelegate[0]));
    if (ER_OK != status) {
        NSLog(@"ERROR: An error occurred while adding method handlers for interface org.alljoyn.alljoyn_test to the interface description. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    
    // save off signal members for later
    //
    my_signalSignalMember = interfaceDescription->GetMember("my_signal");
    assert(my_signalSignalMember);    

    // Add the org.alljoyn.alljoyn_test.values interface to this object
    //
    interfaceDescription = bus.GetInterface("org.alljoyn.alljoyn_test.values");
    assert(interfaceDescription);
    AddInterface(*interfaceDescription);

    

}


QStatus PingObjectImpl::Get(const char* ifcName, const char* propName, MsgArg& val)
{
    QStatus status = ER_BUS_NO_SUCH_PROPERTY;
    
    @autoreleasepool {
    
    if (strcmp(ifcName, "org.alljoyn.alljoyn_test.values") == 0) 
    {
    
        if (strcmp(propName, "int_val") == 0)
        {
                
            status = val.Set( "i", [((id<PingObjectValuesDelegate>)delegate).int_val intValue] );
            
        }
    
        if (strcmp(propName, "ro_str") == 0)
        {
                
            status = val.Set( "s", [((id<PingObjectValuesDelegate>)delegate).ro_str UTF8String] );
            
        }
    
        if (strcmp(propName, "str_val") == 0)
        {
                
            status = val.Set( "s", [((id<PingObjectValuesDelegate>)delegate).str_val UTF8String] );
            
        }
    
    }
    
    
    }
    
    return status;
}
    
QStatus PingObjectImpl::Set(const char* ifcName, const char* propName, MsgArg& val)
{
    QStatus status = ER_BUS_NO_SUCH_PROPERTY;
    
    @autoreleasepool {
    
    if (strcmp(ifcName, "org.alljoyn.alljoyn_test.values") == 0)
    {
    
        if (strcmp(propName, "int_val") == 0)
        {
        int32_t propValue;
            status = val.Get("i", &propValue);
            ((id<PingObjectValuesDelegate>)delegate).int_val = [NSNumber numberWithInt:propValue];
            
        }    
    
        if (strcmp(propName, "str_val") == 0)
        {
        char * propValue;
            status = val.Get("s", &propValue);
            ((id<PingObjectValuesDelegate>)delegate).str_val = [NSString stringWithCString:propValue encoding:NSUTF8StringEncoding];
            
        }    
    
    }
    
    
    }

    return status;
}

void PingObjectImpl::delayed_ping(const InterfaceDescription::Member *member, Message& msg)
{
    @autoreleasepool {
    
    
    
    
    // get all input arguments
    //
    
    qcc::String inArg0 = msg->GetArg(0)->v_string.str;
        
    uint32_t inArg1 = msg->GetArg(1)->v_uint32;
        
    // declare the output arguments
    //
    
	NSString* outArg0;

    
    // call the Objective-C delegate method
    //
    
	outArg0 = [(id<PingObjectDelegate>)delegate sendPingString:[NSString stringWithCString:inArg0.c_str() encoding:NSUTF8StringEncoding] withDelay:[NSNumber numberWithUnsignedInt:inArg1]];
            
        
    // formulate the reply
    //
    MsgArg outArgs[1];
    
    outArgs[0].Set("s", [outArg0 UTF8String]);

    QStatus status = MethodReply(msg, outArgs, 1);
    if (ER_OK != status) {
        NSLog(@"ERROR: An error occurred when attempting to send a method reply for delayed_ping. %@", [AJNStatus descriptionForStatusCode:status]);
    }        
    
    
    }
}


void PingObjectImpl::my_ping(const InterfaceDescription::Member *member, Message& msg)
{
    @autoreleasepool {
    
    
    
    
    // get all input arguments
    //
    
    qcc::String inArg0 = msg->GetArg(0)->v_string.str;
        
    // declare the output arguments
    //
    
	NSString* outArg0;

    
    // call the Objective-C delegate method
    //
    
	outArg0 = [(id<PingObjectDelegate>)delegate sendPingString:[NSString stringWithCString:inArg0.c_str() encoding:NSUTF8StringEncoding]];
            
        
    // formulate the reply
    //
    MsgArg outArgs[1];
    
    outArgs[0].Set("s", [outArg0 UTF8String]);

    QStatus status = MethodReply(msg, outArgs, 1);
    if (ER_OK != status) {
        NSLog(@"ERROR: An error occurred when attempting to send a method reply for my_ping. %@", [AJNStatus descriptionForStatusCode:status]);
    }        
    
    
    }
}


void PingObjectImpl::time_ping(const InterfaceDescription::Member *member, Message& msg)
{
    @autoreleasepool {
    
    
    
    
    // get all input arguments
    //
    
    uint32_t inArg0 = msg->GetArg(0)->v_uint32;
        
    uint16_t inArg1 = msg->GetArg(1)->v_uint16;
        
    // declare the output arguments
    //
    
	NSNumber* outArg0;
	NSNumber* outArg1;

    
    // call the Objective-C delegate method
    //
    
	[(id<PingObjectDelegate>)delegate sendPingAtTimeInSeconds:[NSNumber numberWithUnsignedInt:inArg0] timeInMilliseconds:[NSNumber numberWithUnsignedShort:inArg1] receivedTimeInSeconds:&outArg0 receivedTimeInMilliseconds:&outArg1];
            
        
    // formulate the reply
    //
    MsgArg outArgs[2];
    
    outArgs[0].Set("u", [outArg0 unsignedIntValue]);

    outArgs[1].Set("q", [outArg1 unsignedShortValue]);

    QStatus status = MethodReply(msg, outArgs, 2);
    if (ER_OK != status) {
        NSLog(@"ERROR: An error occurred when attempting to send a method reply for time_ping. %@", [AJNStatus descriptionForStatusCode:status]);
    }        
    
    
    }
}


QStatus PingObjectImpl::Sendmy_signal( const char* destination, SessionId sessionId, uint16_t timeToLive, uint8_t flags)
{

    MsgArg args[0];

    

    return Signal(destination, sessionId, *my_signalSignalMember, args, 0, timeToLive, flags);
}



////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  Objective-C Bus Object implementation for AJNPingObject
//
////////////////////////////////////////////////////////////////////////////////

@implementation AJNPingObject

@dynamic handle;

@synthesize int_val = _int_val;
@synthesize ro_str = _ro_str;
@synthesize str_val = _str_val;


- (PingObjectImpl*)busObject
{
    return static_cast<PingObjectImpl*>(self.handle);
}

- (id)initWithBusAttachment:(AJNBusAttachment *)busAttachment onPath:(NSString *)path
{
    self = [super initWithBusAttachment:busAttachment onPath:path];
    if (self) {
        QStatus status;

        status = ER_OK;
        
        AJNInterfaceDescription *interfaceDescription;
        
    
        //
        // PingObjectDelegate interface (org.alljoyn.alljoyn_test)
        //
        // create an interface description, or if that fails, get the interface as it was already created
        //
        interfaceDescription = [busAttachment createInterfaceWithName:@"org.alljoyn.alljoyn_test"];
        
    
        // add the methods to the interface description
        //
    
        status = [interfaceDescription addMethodWithName:@"delayed_ping" inputSignature:@"su" outputSignature:@"s" argumentNames:[NSArray arrayWithObjects:@"outStr",@"delay",@"inStr", nil]];
        
        if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
            @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add method to interface: delayed_ping" userInfo:nil];
        }

        status = [interfaceDescription addMethodWithName:@"my_ping" inputSignature:@"s" outputSignature:@"s" argumentNames:[NSArray arrayWithObjects:@"outStr",@"inStr", nil]];
        
        if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
            @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add method to interface: my_ping" userInfo:nil];
        }

        status = [interfaceDescription addMethodWithName:@"time_ping" inputSignature:@"uq" outputSignature:@"uq" argumentNames:[NSArray arrayWithObjects:@"sendTimeSecs",@"sendTimeMillisecs",@"receivedTimeSecs",@"receivedTimeMillisecs", nil]];
        
        if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
            @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add method to interface: time_ping" userInfo:nil];
        }

        // add the signals to the interface description
        //
    
        status = [interfaceDescription addSignalWithName:@"my_signal" inputSignature:@"" argumentNames:[NSArray arrayWithObjects: nil]];
        
        if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
            @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add signal to interface:  my_signal" userInfo:nil];
        }

    
        [interfaceDescription activate];

        //
        // PingObjectValuesDelegate interface (org.alljoyn.alljoyn_test.values)
        //
        // create an interface description, or if that fails, get the interface as it was already created
        //
        interfaceDescription = [busAttachment createInterfaceWithName:@"org.alljoyn.alljoyn_test.values"];
        
    
        // add the properties to the interface description
        //
    
        status = [interfaceDescription addPropertyWithName:@"int_val" signature:@"i"];
        
        if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
            @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add property to interface:  int_val" userInfo:nil];
        }

        status = [interfaceDescription addPropertyWithName:@"ro_str" signature:@"s"];
        
        if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
            @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add property to interface:  ro_str" userInfo:nil];
        }

        status = [interfaceDescription addPropertyWithName:@"str_val" signature:@"s"];
        
        if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
            @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add property to interface:  str_val" userInfo:nil];
        }

    
        [interfaceDescription activate];


        // create the internal C++ bus object
        //
        PingObjectImpl *busObject = new PingObjectImpl(*((ajn::BusAttachment*)busAttachment.handle), [path UTF8String], (id<PingObjectDelegate, PingObjectValuesDelegate>)self);
        
        self.handle = busObject;
    }
    return self;
}

- (void)dealloc
{
    PingObjectImpl *busObject = [self busObject];
    delete busObject;
    self.handle = nil;
}

    
- (NSString*)sendPingString:(NSString*)outStr withDelay:(NSNumber*)delay
{
    //
    // GENERATED CODE - DO NOT EDIT
    //
    // Create a category or subclass in separate .h/.m files
    @throw([NSException exceptionWithName:@"NotImplementedException" reason:@"You must override this method in a subclass" userInfo:nil]);
}

- (NSString*)sendPingString:(NSString*)outStr
{
    //
    // GENERATED CODE - DO NOT EDIT
    //
    // Create a category or subclass in separate .h/.m files
    @throw([NSException exceptionWithName:@"NotImplementedException" reason:@"You must override this method in a subclass" userInfo:nil]);
}

- (void)sendPingAtTimeInSeconds:(NSNumber*)sendTimeSecs timeInMilliseconds:(NSNumber*)sendTimeMillisecs receivedTimeInSeconds:(NSNumber**)receivedTimeSecs receivedTimeInMilliseconds:(NSNumber**)receivedTimeMillisecs
{
    //
    // GENERATED CODE - DO NOT EDIT
    //
    // Create a category or subclass in separate .h/.m files
    @throw([NSException exceptionWithName:@"NotImplementedException" reason:@"You must override this method in a subclass" userInfo:nil]);
}
- (void)sendmy_signalInSession:(AJNSessionId)sessionId toDestination:(NSString*)destinationPath

{
    
    self.busObject->Sendmy_signal([destinationPath UTF8String], sessionId);
        
}

    
@end

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  Objective-C Proxy Bus Object implementation for PingObject
//
////////////////////////////////////////////////////////////////////////////////

@interface PingObjectProxy(Private)

@property (nonatomic, strong) AJNBusAttachment *bus;

- (ProxyBusObject*)proxyBusObject;

@end

@implementation PingObjectProxy
    
- (NSNumber*)int_val
{
    [self addInterfaceNamed:@"org.alljoyn.alljoyn_test.values"];
    
    
    MsgArg propValue;
    
    QStatus status = self.proxyBusObject->GetProperty("org.alljoyn.alljoyn_test.values", "int_val", propValue);

    if (status != ER_OK) {
        NSLog(@"ERROR: Failed to get property int_val on interface org.alljoyn.alljoyn_test.values. %@", [AJNStatus descriptionForStatusCode:status]);
    }

    
    return [NSNumber numberWithInt:propValue.v_variant.val->v_int32];
        
}
    
- (void)setInt_val:(NSNumber*)propertyValue
{
    [self addInterfaceNamed:@"org.alljoyn.alljoyn_test.values"];
    
    
    MsgArg arg;

    QStatus status = arg.Set("i", [propertyValue intValue]);    
    if (status != ER_OK) {
        NSLog(@"ERROR: Failed to set property int_val on interface org.alljoyn.alljoyn_test.values. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    
    self.proxyBusObject->SetProperty("org.alljoyn.alljoyn_test.values", "int_val", arg); 
        
    
}
    
- (NSString*)ro_str
{
    [self addInterfaceNamed:@"org.alljoyn.alljoyn_test.values"];
    
    
    MsgArg propValue;
    
    QStatus status = self.proxyBusObject->GetProperty("org.alljoyn.alljoyn_test.values", "ro_str", propValue);

    if (status != ER_OK) {
        NSLog(@"ERROR: Failed to get property ro_str on interface org.alljoyn.alljoyn_test.values. %@", [AJNStatus descriptionForStatusCode:status]);
    }

    
    return [NSString stringWithCString:propValue.v_variant.val->v_string.str encoding:NSUTF8StringEncoding];
        
}
    
- (NSString*)str_val
{
    [self addInterfaceNamed:@"org.alljoyn.alljoyn_test.values"];
    
    
    MsgArg propValue;
    
    QStatus status = self.proxyBusObject->GetProperty("org.alljoyn.alljoyn_test.values", "str_val", propValue);

    if (status != ER_OK) {
        NSLog(@"ERROR: Failed to get property str_val on interface org.alljoyn.alljoyn_test.values. %@", [AJNStatus descriptionForStatusCode:status]);
    }

    
    return [NSString stringWithCString:propValue.v_variant.val->v_string.str encoding:NSUTF8StringEncoding];
        
}
    
- (void)setStr_val:(NSString*)propertyValue
{
    [self addInterfaceNamed:@"org.alljoyn.alljoyn_test.values"];
    
    
    MsgArg arg;

    QStatus status = arg.Set("s", [propertyValue UTF8String]);    
    if (status != ER_OK) {
        NSLog(@"ERROR: Failed to set property str_val on interface org.alljoyn.alljoyn_test.values. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    
    self.proxyBusObject->SetProperty("org.alljoyn.alljoyn_test.values", "str_val", arg); 
        
    
}
    
- (NSString*)sendPingString:(NSString*)outStr withDelay:(NSNumber*)delay
{
    [self addInterfaceNamed:@"org.alljoyn.alljoyn_test"];
    
    // prepare the input arguments
    //
    
    Message reply(*((BusAttachment*)self.bus.handle));    
    MsgArg inArgs[2];
    
    inArgs[0].Set("s", [outStr UTF8String]);

    inArgs[1].Set("u", [delay unsignedIntValue]);


    // make the function call using the C++ proxy object
    //
    
    QStatus status = self.proxyBusObject->MethodCall("org.alljoyn.alljoyn_test", "delayed_ping", inArgs, 2, reply, 5000);
    if (ER_OK != status) {
        NSLog(@"ERROR: ProxyBusObject::MethodCall on org.alljoyn.alljoyn_test failed. %@", [AJNStatus descriptionForStatusCode:status]);
        
        return nil;
            
    }

    
    // pass the output arguments back to the caller
    //
    
        
    return [NSString stringWithCString:reply->GetArg()->v_string.str encoding:NSUTF8StringEncoding];
        

}

- (NSString*)sendPingString:(NSString*)outStr
{
    [self addInterfaceNamed:@"org.alljoyn.alljoyn_test"];
    
    // prepare the input arguments
    //
    
    Message reply(*((BusAttachment*)self.bus.handle));    
    MsgArg inArgs[1];
    
    inArgs[0].Set("s", [outStr UTF8String]);


    // make the function call using the C++ proxy object
    //
    
    QStatus status = self.proxyBusObject->MethodCall("org.alljoyn.alljoyn_test", "my_ping", inArgs, 1, reply, 5000);
    if (ER_OK != status) {
        NSLog(@"ERROR: ProxyBusObject::MethodCall on org.alljoyn.alljoyn_test failed. %@", [AJNStatus descriptionForStatusCode:status]);
        
        return nil;
            
    }

    
    // pass the output arguments back to the caller
    //
    
        
    return [NSString stringWithCString:reply->GetArg()->v_string.str encoding:NSUTF8StringEncoding];
        

}

- (void)sendPingAtTimeInSeconds:(NSNumber*)sendTimeSecs timeInMilliseconds:(NSNumber*)sendTimeMillisecs receivedTimeInSeconds:(NSNumber**)receivedTimeSecs receivedTimeInMilliseconds:(NSNumber**)receivedTimeMillisecs
{
    [self addInterfaceNamed:@"org.alljoyn.alljoyn_test"];
    
    // prepare the input arguments
    //
    
    Message reply(*((BusAttachment*)self.bus.handle));    
    MsgArg inArgs[2];
    
    inArgs[0].Set("u", [sendTimeSecs unsignedIntValue]);

    inArgs[1].Set("q", [sendTimeMillisecs unsignedShortValue]);


    // make the function call using the C++ proxy object
    //
    
    QStatus status = self.proxyBusObject->MethodCall("org.alljoyn.alljoyn_test", "time_ping", inArgs, 2, reply, 5000);
    if (ER_OK != status) {
        NSLog(@"ERROR: ProxyBusObject::MethodCall on org.alljoyn.alljoyn_test failed. %@", [AJNStatus descriptionForStatusCode:status]);
        
        return;
            
    }

    
    // pass the output arguments back to the caller
    //
    
        
    *receivedTimeSecs = [NSNumber numberWithUnsignedInt:reply->GetArg()->v_uint32];
        
    *receivedTimeMillisecs = [NSNumber numberWithUnsignedShort:reply->GetArg()->v_uint16];
        

}

@end

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  C++ Signal Handler implementation for PingObjectDelegate
//
////////////////////////////////////////////////////////////////////////////////

class PingObjectDelegateSignalHandlerImpl : public AJNSignalHandlerImpl
{
private:

    const ajn::InterfaceDescription::Member* my_signalSignalMember;
    void my_signalSignalHandler(const ajn::InterfaceDescription::Member* member, const char* srcPath, ajn::Message& msg);

    
public:
    /**
     * Constructor for the AJN signal handler implementation.
     *
     * @param aDelegate         Objective C delegate called when one of the below virtual functions is called.     
     */    
    PingObjectDelegateSignalHandlerImpl(id<AJNSignalHandler> aDelegate);
    
    virtual void RegisterSignalHandler(ajn::BusAttachment &bus);
    
    virtual void UnregisterSignalHandler(ajn::BusAttachment &bus);
    
    /**
     * Virtual destructor for derivable class.
     */
    virtual ~PingObjectDelegateSignalHandlerImpl();
};


/**
 * Constructor for the AJN signal handler implementation.
 *
 * @param aDelegate         Objective C delegate called when one of the below virtual functions is called.     
 */    
PingObjectDelegateSignalHandlerImpl::PingObjectDelegateSignalHandlerImpl(id<AJNSignalHandler> aDelegate) : AJNSignalHandlerImpl(aDelegate)
{
	my_signalSignalMember = NULL;

}

PingObjectDelegateSignalHandlerImpl::~PingObjectDelegateSignalHandlerImpl()
{
    m_delegate = NULL;
}

void PingObjectDelegateSignalHandlerImpl::RegisterSignalHandler(ajn::BusAttachment &bus)
{
    QStatus status;
    status = ER_OK;
    const ajn::InterfaceDescription* interface = NULL;
    
    ////////////////////////////////////////////////////////////////////////////
    // Register signal handler for signal my_signal
    //
    interface = bus.GetInterface("org.alljoyn.alljoyn_test");

    if (interface) {
        // Store the my_signal signal member away so it can be quickly looked up
        my_signalSignalMember = interface->GetMember("my_signal");
        assert(my_signalSignalMember);

        
        // Register signal handler for my_signal
        status =  bus.RegisterSignalHandler(this,
            static_cast<MessageReceiver::SignalHandler>(&PingObjectDelegateSignalHandlerImpl::my_signalSignalHandler),
            my_signalSignalMember,
            NULL);
            
        if (status != ER_OK) {
            NSLog(@"ERROR: Interface PingObjectDelegateSignalHandlerImpl::RegisterSignalHandler failed. %@", [AJNStatus descriptionForStatusCode:status] );
        }
    }
    else {
        NSLog(@"ERROR: org.alljoyn.alljoyn_test not found.");
    }
    ////////////////////////////////////////////////////////////////////////////    

}

void PingObjectDelegateSignalHandlerImpl::UnregisterSignalHandler(ajn::BusAttachment &bus)
{
    QStatus status;
    status = ER_OK;
    const ajn::InterfaceDescription* interface = NULL;
    
    ////////////////////////////////////////////////////////////////////////////
    // Unregister signal handler for signal my_signal
    //
    interface = bus.GetInterface("org.alljoyn.alljoyn_test");
    
    // Store the my_signal signal member away so it can be quickly looked up
    my_signalSignalMember = interface->GetMember("my_signal");
    assert(my_signalSignalMember);
    
    // Unregister signal handler for my_signal
    status =  bus.UnregisterSignalHandler(this,
        static_cast<MessageReceiver::SignalHandler>(&PingObjectDelegateSignalHandlerImpl::my_signalSignalHandler),
        my_signalSignalMember,
        NULL);
        
    if (status != ER_OK) {
        NSLog(@"ERROR:PingObjectDelegateSignalHandlerImpl::UnregisterSignalHandler failed. %@", [AJNStatus descriptionForStatusCode:status] );
    }
    ////////////////////////////////////////////////////////////////////////////    

}


void PingObjectDelegateSignalHandlerImpl::my_signalSignalHandler(const ajn::InterfaceDescription::Member* member, const char* srcPath, ajn::Message& msg)
{
    @autoreleasepool {
        
        NSString *from = [NSString stringWithCString:msg->GetSender() encoding:NSUTF8StringEncoding];
        NSString *objectPath = [NSString stringWithCString:msg->GetObjectPath() encoding:NSUTF8StringEncoding];
        AJNSessionId sessionId = msg->GetSessionId();        
        NSLog(@"Received my_signal signal from %@ on path %@ for session id %u [%s > %s]", from, objectPath, msg->GetSessionId(), msg->GetRcvEndpointName(), msg->GetDestination() ? msg->GetDestination() : "broadcast");
        
        dispatch_async(dispatch_get_main_queue(), ^{
            
            [(id<PingObjectDelegateSignalHandler>)m_delegate didReceivemy_signalInSession:sessionId fromSender:from];            
                
        });
        
    }
}


@implementation AJNBusAttachment(PingObjectDelegate)

- (void)registerPingObjectDelegateSignalHandler:(id<PingObjectDelegateSignalHandler>)signalHandler
{
    PingObjectDelegateSignalHandlerImpl *signalHandlerImpl = new PingObjectDelegateSignalHandlerImpl(signalHandler);
    signalHandler.handle = signalHandlerImpl;
    [self registerSignalHandler:signalHandler];
}

@end

////////////////////////////////////////////////////////////////////////////////
    