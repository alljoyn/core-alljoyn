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
//  AJNFileTransferObject.mm
//
////////////////////////////////////////////////////////////////////////////////

#import <alljoyn/BusAttachment.h>
#import <alljoyn/BusObject.h>
#import "AJNBusObjectImpl.h"
#import "AJNInterfaceDescription.h"
#import "AJNMessageArgument.h"
#import "AJNSignalHandlerImpl.h"

#import "FileTransferObject.h"

using namespace ajn;

////////////////////////////////////////////////////////////////////////////////
//
//  C++ Bus Object class declaration for FileTransferObjectImpl
//
////////////////////////////////////////////////////////////////////////////////
class FileTransferObjectImpl : public AJNBusObjectImpl
{
private:
    const InterfaceDescription::Member* FileTransferSignalMember;

    
public:
    FileTransferObjectImpl(BusAttachment &bus, const char *path, id<FileTransferDelegate> aDelegate);

    
    
    // methods
    //
    
    
    // signals
    //
    QStatus SendFileTransfer(const char * name,uint32_t curr,MsgArg* data, const char* destination, SessionId sessionId, uint16_t timeToLive = 0, uint8_t flags = 0);

};
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  C++ Bus Object implementation for FileTransferObjectImpl
//
////////////////////////////////////////////////////////////////////////////////

FileTransferObjectImpl::FileTransferObjectImpl(BusAttachment &bus, const char* path, id<FileTransferDelegate> aDelegate) : 
    AJNBusObjectImpl(bus,path,aDelegate)
{
    const InterfaceDescription* interfaceDescription = NULL;
    QStatus status;
    status = ER_OK;
    
    
    // Add the org.alljoyn.bus.samples.fileTransfer interface to this object
    //
    interfaceDescription = bus.GetInterface("org.alljoyn.bus.samples.fileTransfer");
    assert(interfaceDescription);
    AddInterface(*interfaceDescription);

    
    // save off signal members for later
    //
    FileTransferSignalMember = interfaceDescription->GetMember("FileTransfer");
    assert(FileTransferSignalMember);    


}


QStatus FileTransferObjectImpl::SendFileTransfer(const char* name, uint32_t curr, MsgArg* data, const char* destination, SessionId sessionId, uint16_t timeToLive, uint8_t flags)
{

    MsgArg args[3];

    
    args[0].Set( "s", name );

    args[1].Set( "u", curr );

    args[2] = *data;


    return Signal(destination, sessionId, *FileTransferSignalMember, args, 3, timeToLive, flags);
}



////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  Objective-C Bus Object implementation for AJNFileTransferObject
//
////////////////////////////////////////////////////////////////////////////////

@implementation AJNFileTransferObject

@dynamic handle;



- (FileTransferObjectImpl*)busObject
{
    return static_cast<FileTransferObjectImpl*>(self.handle);
}

- (id)initWithBusAttachment:(AJNBusAttachment *)busAttachment onPath:(NSString *)path
{
    self = [super initWithBusAttachment:busAttachment onPath:path];
    if (self) {
        QStatus status;

        status = ER_OK;
        
        AJNInterfaceDescription *interfaceDescription;
        
    
        //
        // FileTransferDelegate interface (org.alljoyn.bus.samples.fileTransfer)
        //
        // create an interface description, or if that fails, get the interface as it was already created
        //
        interfaceDescription = [busAttachment createInterfaceWithName:@"org.alljoyn.bus.samples.fileTransfer"];
        
    
        // add the signals to the interface description
        //
    
        status = [interfaceDescription addSignalWithName:@"FileTransfer" inputSignature:@"suay" argumentNames:[NSArray arrayWithObjects:@"name",@"curr",@"data", nil]];
        
        if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
            @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add signal to interface:  FileTransfer" userInfo:nil];
        }

    
        [interfaceDescription activate];


        // create the internal C++ bus object
        //
        FileTransferObjectImpl *busObject = new FileTransferObjectImpl(*((ajn::BusAttachment*)busAttachment.handle), [path UTF8String], (id<FileTransferDelegate>)self);
        
        self.handle = busObject;
    }
    return self;
}

- (void)dealloc
{
    FileTransferObjectImpl *busObject = [self busObject];
    delete busObject;
    self.handle = nil;
}

- (void)sendTransferFileNamed:(NSString*)name currentIndex:(NSNumber*)curr fileData:(AJNMessageArgument*)data inSession:(AJNSessionId)sessionId toDestination:(NSString*)destinationPath flags:(uint8_t)flags
{
    
    self.busObject->SendFileTransfer([name UTF8String], [curr unsignedIntValue], (MsgArg*)[data handle], [destinationPath UTF8String], sessionId, flags);
        
}

    
@end

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  Objective-C Proxy Bus Object implementation for FileTransferObject
//
////////////////////////////////////////////////////////////////////////////////

@interface FileTransferObjectProxy(Private)

@property (nonatomic, strong) AJNBusAttachment *bus;

- (ProxyBusObject*)proxyBusObject;

@end

@implementation FileTransferObjectProxy
    
@end

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  C++ Signal Handler implementation for FileTransferDelegate
//
////////////////////////////////////////////////////////////////////////////////

class FileTransferDelegateSignalHandlerImpl : public AJNSignalHandlerImpl
{
private:

    const ajn::InterfaceDescription::Member* FileTransferSignalMember;
    void FileTransferSignalHandler(const ajn::InterfaceDescription::Member* member, const char* srcPath, ajn::Message& msg);

    
public:
    /**
     * Constructor for the AJN signal handler implementation.
     *
     * @param aDelegate         Objective C delegate called when one of the below virtual functions is called.     
     */    
    FileTransferDelegateSignalHandlerImpl(id<AJNSignalHandler> aDelegate);
    
    virtual void RegisterSignalHandler(ajn::BusAttachment &bus);
    
    virtual void UnregisterSignalHandler(ajn::BusAttachment &bus);
    
    /**
     * Virtual destructor for derivable class.
     */
    virtual ~FileTransferDelegateSignalHandlerImpl();
};


/**
 * Constructor for the AJN signal handler implementation.
 *
 * @param aDelegate         Objective C delegate called when one of the below virtual functions is called.     
 */    
FileTransferDelegateSignalHandlerImpl::FileTransferDelegateSignalHandlerImpl(id<AJNSignalHandler> aDelegate) : AJNSignalHandlerImpl(aDelegate)
{
	FileTransferSignalMember = NULL;

}

FileTransferDelegateSignalHandlerImpl::~FileTransferDelegateSignalHandlerImpl()
{
    m_delegate = NULL;
}

void FileTransferDelegateSignalHandlerImpl::RegisterSignalHandler(ajn::BusAttachment &bus)
{
    QStatus status;
    status = ER_OK;
    const ajn::InterfaceDescription* interface = NULL;
    
    ////////////////////////////////////////////////////////////////////////////
    // Register signal handler for signal FileTransfer
    //
    interface = bus.GetInterface("org.alljoyn.bus.samples.fileTransfer");

    if (interface) {
        // Store the FileTransfer signal member away so it can be quickly looked up
        FileTransferSignalMember = interface->GetMember("FileTransfer");
        assert(FileTransferSignalMember);

        
        // Register signal handler for FileTransfer
        status =  bus.RegisterSignalHandler(this,
            static_cast<MessageReceiver::SignalHandler>(&FileTransferDelegateSignalHandlerImpl::FileTransferSignalHandler),
            FileTransferSignalMember,
            NULL);
            
        if (status != ER_OK) {
            NSLog(@"ERROR: Interface FileTransferDelegateSignalHandlerImpl::RegisterSignalHandler failed. %@", [AJNStatus descriptionForStatusCode:status] );
        }
    }
    else {
        NSLog(@"ERROR: org.alljoyn.bus.samples.fileTransfer not found.");
    }
    ////////////////////////////////////////////////////////////////////////////    

}

void FileTransferDelegateSignalHandlerImpl::UnregisterSignalHandler(ajn::BusAttachment &bus)
{
    QStatus status;
    status = ER_OK;
    const ajn::InterfaceDescription* interface = NULL;
    
    ////////////////////////////////////////////////////////////////////////////
    // Unregister signal handler for signal FileTransfer
    //
    interface = bus.GetInterface("org.alljoyn.bus.samples.fileTransfer");
    
    // Store the FileTransfer signal member away so it can be quickly looked up
    FileTransferSignalMember = interface->GetMember("FileTransfer");
    assert(FileTransferSignalMember);
    
    // Unregister signal handler for FileTransfer
    status =  bus.UnregisterSignalHandler(this,
        static_cast<MessageReceiver::SignalHandler>(&FileTransferDelegateSignalHandlerImpl::FileTransferSignalHandler),
        FileTransferSignalMember,
        NULL);
        
    if (status != ER_OK) {
        NSLog(@"ERROR:FileTransferDelegateSignalHandlerImpl::UnregisterSignalHandler failed. %@", [AJNStatus descriptionForStatusCode:status] );
    }
    ////////////////////////////////////////////////////////////////////////////    

}


void FileTransferDelegateSignalHandlerImpl::FileTransferSignalHandler(const ajn::InterfaceDescription::Member* member, const char* srcPath, ajn::Message& msg)
{
    @autoreleasepool {
        
        qcc::String inArg0 = msg->GetArg(0)->v_string.str;
            
        uint32_t inArg1 = msg->GetArg(1)->v_uint32;
        
        MsgArg *pArg2 = new MsgArg(*(msg->GetArg(2)));
            
        AJNMessageArgument* inArg2 = [[AJNMessageArgument alloc] initWithHandle:(AJNHandle)pArg2 shouldDeleteHandleOnDealloc:YES];
        
        NSString *from = [NSString stringWithCString:msg->GetSender() encoding:NSUTF8StringEncoding];
        NSString *objectPath = [NSString stringWithCString:msg->GetObjectPath() encoding:NSUTF8StringEncoding];
        AJNSessionId sessionId = msg->GetSessionId();        
        NSLog(@"Received FileTransfer signal from %@ on path %@ for session id %u [%s > %s]", from, objectPath, msg->GetSessionId(), msg->GetRcvEndpointName(), msg->GetDestination() ? msg->GetDestination() : "broadcast");
        
        dispatch_async(dispatch_get_main_queue(), ^{
            
            [(id<FileTransferDelegateSignalHandler>)m_delegate didReceiveTransferFileNamed:[NSString stringWithCString:inArg0.c_str() encoding:NSUTF8StringEncoding] currentIndex:[NSNumber numberWithUnsignedInt:inArg1] fileData:inArg2 inSession:sessionId fromSender:from];
                
        });
        
    }
}


@implementation AJNBusAttachment(FileTransferDelegate)

- (void)registerFileTransferDelegateSignalHandler:(id<FileTransferDelegateSignalHandler>)signalHandler
{
    FileTransferDelegateSignalHandlerImpl *signalHandlerImpl = new FileTransferDelegateSignalHandlerImpl(signalHandler);
    signalHandler.handle = signalHandlerImpl;
    [self registerSignalHandler:signalHandler];
}

@end

////////////////////////////////////////////////////////////////////////////////
    