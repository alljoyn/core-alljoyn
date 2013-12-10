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

#import <objc/runtime.h>
#import <qcc/String.h>
#import <alljoyn/BusAttachment.h>
#import <alljoyn/BusObject.h>
#import <alljoyn/DBusStd.h>
#import <alljoyn/InterfaceDescription.h>

#import "AJNAuthenticationListenerImpl.h"
#import "AJNBusAttachment.h"
#import "AJNBusInterface.h"
#import "AJNBusListenerImpl.h"
#import "AJNBusObject.h"
#import "AJNBusObjectImpl.h"
#import "AJNInterfaceDescription.h"
#import "AJNInterfaceMember.h"
#import "AJNKeyStoreListenerImpl.h"
#import "AJNProxyBusObject.h"
#import "AJNSessionListenerImpl.h"
#import "AJNSessionPortListenerImpl.h"
#import "AJNSignalHandler.h"
#import "AJNSignalHandlerImpl.h"

using namespace ajn;

////////////////////////////////////////////////////////////////////////////////
//
//  Asynchronous session join callback implementation
//

/** Internal class to manage asynchronous join session callbacks
 */
class AJNSetLinkTimeoutAsyncCallbackImpl : public ajn::BusAttachment::SetLinkTimeoutAsyncCB
{
private:
    /** The objective-c delegate to communicate with whenever JoinSessionCB is called.
     */
    __weak id<AJNLinkTimeoutDelegate> m_delegate;
    
    /** The objective-c block to call when JoinSessionCB is called.
     */
    AJNLinkTimeoutBlock m_block;
public:
    /** Constructors */
    AJNSetLinkTimeoutAsyncCallbackImpl(id<AJNLinkTimeoutDelegate> delegate) : m_delegate(delegate) { }
    
    AJNSetLinkTimeoutAsyncCallbackImpl(AJNLinkTimeoutBlock block) : m_block(block) { }
    
    /** Destructor */
    virtual ~AJNSetLinkTimeoutAsyncCallbackImpl() 
    { 
        m_delegate = nil;
        m_block = nil;
    }
    
    /**
     * Called when SetLinkTimeoutAsync() completes.
     *
     * @param status       ER_OK if successful
     * @param timeout      Timeout value (possibly adjusted from original request).
     * @param context      User defined context which will be passed as-is to callback.
     */
    virtual void SetLinkTimeoutCB(QStatus status, uint32_t timeout, void* context)
    {
        if (m_delegate != nil) {
            __block id<AJNLinkTimeoutDelegate> theDelegate = m_delegate;
            
            dispatch_async(dispatch_get_main_queue(), ^{
                [theDelegate didSetLinkTimeoutTo:timeout status:status context:context];
                delete this;                
            });
        }
        else {
            dispatch_async(dispatch_get_main_queue(), ^{
                m_block(status, timeout, context);
                delete this;
            });
        }
    }
    
};
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  Asynchronous session join callback implementation
//

/** Internal class to manage asynchronous join session callbacks
 */
class AJNJoinSessionAsyncCallbackImpl : public ajn::BusAttachment::JoinSessionAsyncCB
{
private:
    /** The objective-c delegate to communicate with whenever JoinSessionCB is called.
     */
    __weak id<AJNSessionDelegate> m_delegate;
    
    /** The objective-c block to call when JoinSessionCB is called.
     */
    AJNJoinSessionBlock m_block;
public:
    /** Constructors */
    AJNJoinSessionAsyncCallbackImpl(id<AJNSessionDelegate> delegate) : m_delegate(delegate) { }

    AJNJoinSessionAsyncCallbackImpl(AJNJoinSessionBlock block) : m_block(block) { }

    /** Destructor */
    virtual ~AJNJoinSessionAsyncCallbackImpl() 
    { 
        m_delegate = nil;
        m_block = nil;
    }
    
    /**
     * Called when JoinSessionAsync() completes.
     *
     * @param status       ER_OK if successful
     * @param sessionId    Unique identifier for session.
     * @param opts         Session options.
     * @param context      User defined context which will be passed as-is to callback.
     */
    virtual void JoinSessionCB(QStatus status, SessionId sessionId, const SessionOpts& opts, void* context)
    {
        if (m_delegate != nil) {
            __block id<AJNSessionDelegate> theDelegate = m_delegate;
            
            dispatch_async(dispatch_get_main_queue(), ^{
                [theDelegate didJoinSession:sessionId status:status sessionOptions:[[AJNSessionOptions alloc] initWithHandle:(AJNHandle)&opts] context:context];
                delete this;                
            });
        }
        else {
            dispatch_async(dispatch_get_main_queue(), ^{
                m_block(status, sessionId, [[AJNSessionOptions alloc] initWithHandle:(AJNHandle)&opts], context);
                delete this;
            });
        }
    }
    
};
////////////////////////////////////////////////////////////////////////////////

@interface AJNBusAttachment()

@property (nonatomic, strong) NSMutableArray *busListeners;
@property (nonatomic, strong) NSMutableArray *sessionListeners;
@property (nonatomic, strong) NSMutableArray *sessionPortListeners;
@property (nonatomic, strong) NSMutableArray *signalHandlers;
@property (nonatomic, strong) NSMutableArray *keyStoreListeners;
@property (nonatomic, strong) NSMutableArray *authenticationListeners;

/** C++ AllJoyn API object
 */
@property (nonatomic, readonly) BusAttachment *busAttachment;

@end

@implementation AJNBusAttachment

@synthesize busListeners = _busListeners;
@synthesize sessionListeners = _sessionListeners;
@synthesize sessionPortListeners = _sessionPortListeners;
@synthesize signalHandlers = _signalHandlers;
@synthesize keyStoreListeners = _keyStoreListeners;
@synthesize authenticationListeners = _authenticationListeners;

/** Accessor for the internal C++ API object this objective-c class encapsulates
 */
- (ajn::BusAttachment*)busAttachment
{
    return static_cast<ajn::BusAttachment*>(self.handle);
}

- (NSMutableArray*)busListeners
{
    if (_busListeners == nil) {
        _busListeners = [[NSMutableArray alloc] init];
    }
    return _busListeners;
}

- (NSMutableArray*)sessionListeners
{
    if (_sessionListeners == nil) {
        _sessionListeners = [[NSMutableArray alloc] init];
    }
    return _sessionListeners;
}

- (NSMutableArray*)sessionPortListeners
{
    if (_sessionPortListeners == nil) {
        _sessionPortListeners = [[NSMutableArray alloc] init];
    }
    return _sessionPortListeners;
}

- (NSMutableArray*)signalHandlers
{
    if (_signalHandlers == nil) {
        _signalHandlers = [[NSMutableArray alloc] init];
    }
    return _signalHandlers;
}

- (NSMutableArray*)keyStoreListeners
{
    if (_keyStoreListeners == nil) {
        _keyStoreListeners = [[NSMutableArray alloc] init];
    }
    return _keyStoreListeners;
}

- (NSMutableArray*)authenticationListeners
{
    if (_authenticationListeners == nil) {
        _authenticationListeners = [[NSMutableArray alloc] init];
    }
    return _authenticationListeners;
}

- (BOOL)isConnected
{
    return self.busAttachment->IsConnected();
}

- (BOOL)isStarted
{
    return self.busAttachment->IsStarted();
}

- (BOOL)isStopping
{
    return self.busAttachment->IsStopping();
}

- (NSUInteger)concurrency
{
    return self.busAttachment->GetConcurrency();
}

- (NSString*)uniqueName
{
    return [NSString stringWithCString:self.busAttachment->GetUniqueName().c_str() encoding:NSUTF8StringEncoding];
}

- (NSString*)uniqueIdentifier
{
    return [NSString stringWithCString:self.busAttachment->GetGlobalGUIDString().c_str() encoding:NSUTF8StringEncoding];
}

- (BOOL)isPeerSecurityEnabled
{
    return self.busAttachment->IsPeerSecurityEnabled();
}

- (AJNProxyBusObject*)dbusProxyObject
{
    return [[AJNProxyBusObject alloc] initWithHandle:(AJNHandle)(&self.busAttachment->GetDBusProxyObj())];
}

- (AJNProxyBusObject*)allJoynProxyObject
{
    return [[AJNProxyBusObject alloc] initWithHandle:(AJNHandle)(&self.busAttachment->GetAllJoynProxyObj())];
}

- (AJNProxyBusObject*)allJoynDebugProxyObject
{
    return [[AJNProxyBusObject alloc] initWithHandle:(AJNHandle)(&self.busAttachment->GetAllJoynDebugObj())];
}

- (NSArray *)interfaces
{
    NSArray * returnValue = nil;
    size_t interfaceCount = self.busAttachment->GetInterfaces();
    if (interfaceCount) {
        NSMutableArray *interfacesActivated = [[NSMutableArray alloc] init];
        InterfaceDescription **pInterfaces = new InterfaceDescription*[interfaceCount];
        self.busAttachment->GetInterfaces(const_cast<const InterfaceDescription**>(pInterfaces), interfaceCount);
        for (int i = 0; i < interfaceCount; i++) {
            [interfacesActivated addObject:[[AJNInterfaceDescription alloc] initWithHandle:pInterfaces[i]shouldDeleteHandleOnDealloc:NO]];
        }
        delete [] pInterfaces;
        returnValue = interfacesActivated;
    }
    return returnValue;
}

- (id)initWithApplicationName:(NSString*)applicationName allowRemoteMessages:(BOOL)allowRemoteMessages
{
    self = [super init];
    if (self) {
        self.handle = new ajn::BusAttachment([applicationName UTF8String], allowRemoteMessages);
    }
    return self;
}

- (id)initWithApplicationName:(NSString*)applicationName allowRemoteMessages:(BOOL)allowRemoteMessages maximumConcurrentOperations:(NSUInteger)maximumConcurrentOperations
{
    self = [super init];
    if (self) {
        self.handle = new ajn::BusAttachment([applicationName UTF8String], allowRemoteMessages, maximumConcurrentOperations);
    }
    return self;    
}

- (void)destroy
{
    ajn::BusAttachment* ptr = [self busAttachment];
    if (ptr) {
        delete ptr;
    }
    self.handle = NULL;    
}

/** Destroys all C++ API objects that are maintained in association with this bus attachment
 */
- (void)dealloc
{
    NSLog(@"Deallocating bus attachment...");
    @synchronized(self.busListeners) {
        for (NSValue *ptrValue in self.busListeners) {
            AJNBusListenerImpl * busListenerImpl = (AJNBusListenerImpl*)[ptrValue pointerValue];
            self.busAttachment->UnregisterBusListener(*busListenerImpl);
            delete busListenerImpl;
        }
        [self.busListeners removeAllObjects];
    }

    @synchronized(self.sessionListeners) {
        for (NSValue *ptrValue in self.sessionListeners) {
            AJNSessionListenerImpl * listenerImpl = (AJNSessionListenerImpl*)[ptrValue pointerValue];
            delete listenerImpl;
        }
        [self.sessionListeners removeAllObjects];
    }

    @synchronized(self.sessionPortListeners) {
        for (NSValue *ptrValue in self.sessionPortListeners) {
            AJNSessionPortListenerImpl * listenerImpl = (AJNSessionPortListenerImpl*)[ptrValue pointerValue];
            delete listenerImpl;
        }
        [self.sessionPortListeners removeAllObjects];
    }
    
    @synchronized(self.keyStoreListeners) {
        for (NSValue *ptrValue in self.keyStoreListeners) {
            AJNKeyStoreListenerImpl * listenerImpl = (AJNKeyStoreListenerImpl*)[ptrValue pointerValue];
            delete listenerImpl;
        }
        [self.keyStoreListeners removeAllObjects];
    }

    @synchronized(self.authenticationListeners) {
        for (NSValue *ptrValue in self.authenticationListeners) {
            AJNAuthenticationListenerImpl * listenerImpl = (AJNAuthenticationListenerImpl*)[ptrValue pointerValue];
            delete listenerImpl;
        }
        [self.authenticationListeners removeAllObjects];
    }

    @synchronized(self.signalHandlers) {
        [self.signalHandlers removeAllObjects];
    }

    ajn::BusAttachment* ptr = [self busAttachment];
    if (ptr) {
        delete ptr;
    }
    self.handle = NULL;
}

- (AJNInterfaceDescription*)createInterfaceWithName:(NSString *)interfaceName
{
    return [self createInterfaceWithName:interfaceName enableSecurity:NO];
}

- (AJNInterfaceDescription*)createInterfaceWithName:(NSString *)interfaceName enableSecurity:(BOOL)shouldEnableSecurity
{
    AJNInterfaceDescription *interfaceDescription;
    ajn::InterfaceDescription *handle;
    QStatus status = self.busAttachment->CreateInterface([interfaceName UTF8String], handle, shouldEnableSecurity);
    if (status != ER_OK && status != ER_BUS_IFACE_ALREADY_EXISTS) {
        NSLog(@"ERROR: BusAttachment failed to create interface named %@. %s", interfaceName, QCC_StatusText(status));
    }
    else {
        interfaceDescription = [[AJNInterfaceDescription alloc] initWithHandle:handle];
    }
    return interfaceDescription;
}

- (AJNInterfaceDescription*)interfaceWithName:(NSString*)interfaceName
{
    const ajn::InterfaceDescription *pInterfaceDescription = self.busAttachment->GetInterface([interfaceName UTF8String]);
    
    AJNInterfaceDescription *interfaceDescription;
    
    if (pInterfaceDescription) {
        interfaceDescription = [[AJNInterfaceDescription alloc] initWithHandle:(AJNHandle)pInterfaceDescription];
    }

    return interfaceDescription;
}

- (QStatus)deleteInterfaceWithName:(NSString*)interfaceName
{
    return [self deleteInterface:[self interfaceWithName:interfaceName]];
}

- (QStatus)deleteInterface:(AJNInterfaceDescription*)interfaceDescription
{
    QStatus status = self.busAttachment->DeleteInterface(*static_cast<InterfaceDescription*>(interfaceDescription.handle));
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNBusAttachment::deleteInterface failed. %@", [AJNStatus descriptionForStatusCode:status]);
    }    
    return status;
}

- (QStatus)createInterfacesFromXml:(NSString*)xmlString
{
    QStatus status = self.busAttachment->CreateInterfacesFromXml([xmlString UTF8String]);
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNBusAttachment::createInterfacesFromXml failed. %@", [AJNStatus descriptionForStatusCode:status]);
    }        
    return status;
}

- (void)registerBusListener:(id<AJNBusListener>)delegate
{
    AJNBusListenerImpl * busListenerImpl = new AJNBusListenerImpl(self, delegate);
    @synchronized(self.busListeners) {
        [self.busListeners addObject:[NSValue valueWithPointer:busListenerImpl]];
        self.busAttachment->RegisterBusListener(*busListenerImpl);        
    }
}

- (void)unregisterBusListener:(id<AJNBusListener>)delegate
{
    @synchronized(self.busListeners) {
        for (NSValue *ptrValue in self.busListeners) {
            AJNBusListenerImpl * busListenerImpl = (AJNBusListenerImpl*)[ptrValue pointerValue];
            if (busListenerImpl->getDelegate() == delegate) {
                self.busAttachment->UnregisterBusListener(*busListenerImpl);
                break;
            }
        }
    }    
}

- (void)destroyBusListener:(id<AJNBusListener>)delegate
{
    @synchronized(self.busListeners) {
        NSValue *ptrToRemove = nil;
        for (NSValue *ptrValue in self.busListeners) {
            AJNBusListenerImpl * busListenerImpl = (AJNBusListenerImpl*)[ptrValue pointerValue];
            if (busListenerImpl->getDelegate() == delegate) {
                delete busListenerImpl;
                ptrToRemove = ptrValue;
                break;
            }
        }
        [self.busListeners removeObject:ptrToRemove];
    }    
}

- (void)registerSignalHandler:(id<AJNSignalHandler>)delegate
{
    @synchronized(self.signalHandlers) {
        [self.signalHandlers addObject:delegate];
    }
    
    static_cast<AJNSignalHandlerImpl*>(delegate.handle)->RegisterSignalHandler(*self.busAttachment);    
}

- (void)unregisterSignalHandler:(id<AJNSignalHandler>)delegate
{
    @synchronized(self.signalHandlers) {
        if ([self.signalHandlers containsObject:delegate]) {
            static_cast<AJNSignalHandlerImpl*>(delegate.handle)->UnregisterSignalHandler(*self.busAttachment);
            [self.signalHandlers removeObject:delegate];            
        }
    }        
}

- (void)unregisterAllHandlersForReceiver:(id<AJNHandle>)receiver
{
    if ([receiver conformsToProtocol:@protocol(AJNSignalHandler)]) {
        [self unregisterSignalHandler:(id<AJNSignalHandler>)receiver];
    }
    
    if ([receiver conformsToProtocol:@protocol(AJNBusListener)]) {
        [self unregisterBusListener:(id<AJNBusListener>)receiver];
    }
}

- (QStatus)registerBusObject:(AJNBusObject*)busObject
{
    return [self registerBusObject:busObject enableSecurity:NO];
}

- (QStatus) registerBusObject:(AJNBusObject *) busObject enableSecurity:(BOOL)shouldEnableSecurity
{
    QStatus status = self.busAttachment->RegisterBusObject(*((ajn::BusObject*)busObject.handle), shouldEnableSecurity);
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNBusAttachment::registerBusObject failed. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    return status;
}

- (void)unregisterBusObject:(AJNBusObject*)busObject
{
    self.busAttachment->UnregisterBusObject(*((ajn::BusObject*)busObject.handle));
}

- (QStatus)start
{
    QStatus status = self.busAttachment->Start();
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNBusAttachment::start failed. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    return status;
}

- (QStatus)stop
{
    QStatus status = self.busAttachment->Stop();
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNBusAttachment::stop failed. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    return status;
}

- (QStatus)waitUntilStopCompleted
{
    QStatus status = self.busAttachment->Join();
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNBusAttachment::waitUntilStopCompleted failed. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    return status;    
}

- (void)enableConcurrentCallbacks
{
    self.busAttachment->EnableConcurrentCallbacks();
}

- (QStatus)connectWithArguments:(NSString*)connectionArguments
{
    QStatus status = self.busAttachment->Connect([connectionArguments UTF8String]);
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNBusAttachment::connectWithArguments failed. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    return status;
}

- (QStatus)disconnectWithArguments:(NSString*)connectionArguments
{
    QStatus status = self.busAttachment->Disconnect([connectionArguments UTF8String]);
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNBusAttachment::disconnectWithArguments failed. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    return status;
}

- (QStatus)requestWellKnownName:(NSString *)name withFlags:(AJNBusNameFlag)flags
{
    QStatus status = self.busAttachment->RequestName([name UTF8String], flags);
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNBusAttachment::requestWellKnownName failed. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    return status;    
}

- (QStatus)releaseWellKnownName:(NSString *)name
{
    QStatus status = self.busAttachment->ReleaseName([name UTF8String]);
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNBusAttachment::releaseWellKnownName failed. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    return status;    
}

- (BOOL)doesWellKnownNameHaveOwner:(NSString*)name
{
    bool hasOwner = false;    
    QStatus status = self.busAttachment->NameHasOwner([name UTF8String], hasOwner);
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNBusAttachment::doesWellKnownNameHaveOwner: failed. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    return hasOwner ? YES : NO;
}

- (QStatus)bindSessionOnPort:(AJNSessionPort)port withOptions:(AJNSessionOptions*)options withDelegate:(id<AJNSessionPortListener>)delegate
{
    AJNSessionPortListenerImpl *listenerImpl = new AJNSessionPortListenerImpl(self, delegate);
    QStatus status = self.busAttachment->BindSessionPort(port, *((ajn::SessionOpts*)options.handle), *listenerImpl);
    @synchronized(self.sessionPortListeners) {
        [self.sessionPortListeners addObject:[NSValue valueWithPointer:listenerImpl]];
    }
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNBusAttachment::bindSessionOnPort:withOptions:withDelegate: failed. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    return status;
}

- (AJNSessionPort)bindSessionOnAnyPortWithOptions:(AJNSessionOptions*)options withDelegate:(id<AJNSessionPortListener>)delegate
{
    AJNSessionPortListenerImpl *listenerImpl = new AJNSessionPortListenerImpl(self, delegate);
    AJNSessionPort sessionPort = kAJNSessionPortAny;
    QStatus status = self.busAttachment->BindSessionPort(sessionPort, *((ajn::SessionOpts*)options.handle), *listenerImpl);
    @synchronized(self.sessionPortListeners) {
        [self.sessionPortListeners addObject:[NSValue valueWithPointer:listenerImpl]];
    }
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNBusAttachment::bindSessionOnPort:withOptions:withDelegate: failed. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    return sessionPort;    
}


- (QStatus)unbindSessionFromPort:(AJNSessionPort)port
{
    QStatus status = self.busAttachment->UnbindSessionPort(port);
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNBusAttachment::unbindSessionFromPort: failed. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    return status;
}

- (AJNSessionId)joinSessionWithName:(NSString *)sessionName onPort:(AJNSessionPort)sessionPort withDelegate:(id<AJNSessionListener>)delegate options:(AJNSessionOptions *)options
{
    AJNSessionId sessionId = 0;
    AJNSessionListenerImpl *listenerImpl = new AJNSessionListenerImpl(self, delegate);
    QStatus status = self.busAttachment->JoinSession([sessionName UTF8String], sessionPort, listenerImpl, sessionId, *((ajn::SessionOpts*)options.handle));
    @synchronized(self.sessionListeners) {
        [self.sessionListeners addObject:[NSValue valueWithPointer:listenerImpl]];
    }
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNBusAttachment::joinSessionWithName:onPort:withDelegate:options: failed. %@", [AJNStatus descriptionForStatusCode:status]);
        if (status != ER_ALLJOYN_JOINSESSION_REPLY_ALREADY_JOINED) {
            sessionId = 0;
        }
    }
    return sessionId;    
}

- (QStatus)joinSessionAsyncWithName:(NSString*)sessionName onPort:(AJNSessionPort)sessionPort withDelegate:(id<AJNSessionListener>)delegate options:(AJNSessionOptions*)options joinCompletedDelegate:(id<AJNSessionDelegate>)completionDelegate context:(AJNHandle)context;
{
    AJNSessionListenerImpl *listenerImpl = new AJNSessionListenerImpl(self, delegate);
    AJNJoinSessionAsyncCallbackImpl *callbackImpl = new AJNJoinSessionAsyncCallbackImpl(completionDelegate);
    QStatus status = self.busAttachment->JoinSessionAsync([sessionName UTF8String], sessionPort, listenerImpl, *((ajn::SessionOpts*)options.handle), callbackImpl, context);
    @synchronized(self.sessionListeners) {
        [self.sessionListeners addObject:[NSValue valueWithPointer:listenerImpl]];
    }
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNBusAttachment::joinSessionAsyncWithName:onPort:withDelegate:options:completedDelegate:context: failed. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    return status;    
}

- (QStatus)joinSessionAsyncWithName:(NSString*)sessionName onPort:(AJNSessionPort)sessionPort withDelegate:(id<AJNSessionListener>)delegate options:(AJNSessionOptions*)options joinCompletedBlock:(AJNJoinSessionBlock)completionBlock context:(AJNHandle)context;
{
    AJNSessionListenerImpl *listenerImpl = new AJNSessionListenerImpl(self, delegate);
    AJNJoinSessionAsyncCallbackImpl *callbackImpl = new AJNJoinSessionAsyncCallbackImpl(completionBlock);
    QStatus status = self.busAttachment->JoinSessionAsync([sessionName UTF8String], sessionPort, listenerImpl, *((ajn::SessionOpts*)options.handle), callbackImpl, context);
    @synchronized(self.sessionListeners) {
        [self.sessionListeners addObject:[NSValue valueWithPointer:listenerImpl]];
    }
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNBusAttachment::joinSessionAsyncWithName:onPort:withDelegate:options:completedBlock:context: failed. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    return status;    
}

- (QStatus)bindSessionListener:(id<AJNSessionListener>)delegate toSession:(AJNSessionId)sessionId
{
    AJNSessionListenerImpl *listenerImpl = new AJNSessionListenerImpl(self, delegate);
    QStatus status = self.busAttachment->SetSessionListener(sessionId, listenerImpl);
    @synchronized(self.sessionListeners) {
        [self.sessionListeners addObject:[NSValue valueWithPointer:listenerImpl]];
    }
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNBusAttachment::bindSessionListener:toSession: failed. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    return status;        
}

- (QStatus)leaveSession:(AJNSessionId)sessionId
{
    QStatus status = self.busAttachment->LeaveSession(sessionId);
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNBusAttachment::leaveSession: failed. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    return status;    
}

- (QStatus)removeSessionMember:(AJNSessionId)sessionId withName:(NSString *)memberName
{
    QStatus status = self.busAttachment->RemoveSessionMember(sessionId, [memberName UTF8String]);
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNBusAttachment::RemoveSessionMember: failed. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    return status;
}

- (QStatus)setLinkTimeout:(uint32_t*)timeout forSession:(AJNSessionId)sessionId
{
    QStatus status = self.busAttachment->SetLinkTimeout(sessionId, *timeout);
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNBusAttachment::setLinkTimeout:forSession: failed. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    return status;
}

- (QStatus)setLinkTimeoutAsync:(uint32_t)timeout forSession:(AJNSessionId)sessionId completionBlock:(AJNLinkTimeoutBlock)block context:(void *)context
{
    AJNSetLinkTimeoutAsyncCallbackImpl *callbackImpl = new AJNSetLinkTimeoutAsyncCallbackImpl(block);
    QStatus status = self.busAttachment->SetLinkTimeoutAsync(sessionId, timeout, callbackImpl, context);
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNBusAttachment::setLinkTimeoutAsync:forSession:completionBlock:context: failed. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    return status;    
}

- (QStatus)setLinkTimeoutAsync:(uint32_t)timeout forSession:(AJNSessionId)sessionId completionDelegate:(id<AJNLinkTimeoutDelegate>)delegate context:(void *)context
{
    AJNSetLinkTimeoutAsyncCallbackImpl *callbackImpl = new AJNSetLinkTimeoutAsyncCallbackImpl(delegate);
    QStatus status = self.busAttachment->SetLinkTimeoutAsync(sessionId, timeout, callbackImpl, context);
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNBusAttachment::setLinkTimeoutAsync:forSession:completionDelegate:context: failed. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    return status;    
}

- (AJNHandle)socketFileDescriptorForSession:(AJNSessionId)sessionId
{
    qcc::SocketFd fd;
    QStatus status = self.busAttachment->GetSessionFd(sessionId, fd);
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNBusAttachment::socketFileDescriptorForSession: failed. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    return (AJNHandle)fd;    
}

- (QStatus)advertiseName:(NSString*)name withTransportMask:(AJNTransportMask)mask
{
    QStatus status = self.busAttachment->AdvertiseName([name UTF8String], mask);
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNBusAttacment::advertiseName:withTransportMask: failed. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    return status;
}

- (QStatus)cancelAdvertisedName:(NSString*)name withTransportMask:(AJNTransportMask)mask
{
    QStatus status = self.busAttachment->CancelAdvertiseName([name UTF8String], mask);
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNBusAttacment::cancelAdvertisedName:withTransportMask failed. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    return status;    
}

- (QStatus)findAdvertisedName:(NSString *)name
{
    QStatus status = self.busAttachment->FindAdvertisedName([name UTF8String]);
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNBusAttacment::findAdvertisedName failed. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    return status;
}

- (QStatus)findAdvertisedName:(NSString *)name byTransport:(AJNTransportMask)transports
{
    QStatus status = self.busAttachment->FindAdvertisedNameByTransport([name UTF8String], transports);
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNBusAttacment::findAdvertisedName:byTransport: failed. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    return status;
}


- (QStatus)cancelFindAdvertisedName:(NSString*)name
{
    QStatus status = self.busAttachment->CancelFindAdvertisedName([name UTF8String]);
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNBusAttacment::cancelFindAdvertisedName failed. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    return status;    
}

- (QStatus)cancelFindAdvertisedName:(NSString*)name byTransport:(AJNTransportMask)transports
{
    QStatus status = self.busAttachment->CancelFindAdvertisedNameByTransport([name UTF8String], transports);
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNBusAttacment::cancelFindAdvertisedName:byTransport: failed. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    return status;
}


- (QStatus)addMatchRule:(NSString*)matchRule
{
    QStatus status = self.busAttachment->AddMatch([matchRule UTF8String]);
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNBusAttacment::addMatchRule failed. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    return status;
}

- (QStatus)removeMatchRule:(NSString*)matchRule
{
    QStatus status = self.busAttachment->RemoveMatch([matchRule UTF8String]);
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNBusAttacment::removeMatchRule failed. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    return status;
}

- (QStatus)addLogonEntryToKeyStoreWithAuthenticationMechanism:(NSString *)authenticationMechanism userName:(NSString *)userName password:(NSString *)password
{
    return self.busAttachment->AddLogonEntry([authenticationMechanism UTF8String], [userName UTF8String], [password UTF8String]);
}

- (QStatus)enablePeerSecurity:(NSString*)authenticationMechanisms authenticationListener:(id<AJNAuthenticationListener>)listener
{
    return [self enablePeerSecurity:authenticationMechanisms authenticationListener:listener keystoreFileName:nil sharing:NO];
}

- (QStatus)enablePeerSecurity:(NSString*)authenticationMechanisms authenticationListener:(id<AJNAuthenticationListener>)listener keystoreFileName:(NSString*)fileName sharing:(BOOL)isShared
{
    AJNAuthenticationListenerImpl *listenerImpl = new AJNAuthenticationListenerImpl(listener);
    @synchronized(self.authenticationListeners) {
        [self.authenticationListeners addObject:[NSValue valueWithPointer:listenerImpl]];
    }
    return self.busAttachment->EnablePeerSecurity([authenticationMechanisms UTF8String], listenerImpl, [fileName UTF8String], isShared);
}

- (QStatus)registerKeyStoreListener:(id<AJNKeyStoreListener>)listener
{
    AJNKeyStoreListenerImpl *listenerImpl = new AJNKeyStoreListenerImpl(listener);
    QStatus status=ER_NONE;
    @synchronized(self.keyStoreListeners) {
        [self.keyStoreListeners addObject:[NSValue valueWithPointer:listenerImpl]];
        status = self.busAttachment->RegisterKeyStoreListener(*listenerImpl);        
    }
    return status;
}

- (QStatus)reloadKeyStore
{
    return self.busAttachment->ReloadKeyStore();
}

- (void)clearKeyStore
{
    self.busAttachment->ClearKeyStore();
}

- (QStatus)clearKeysForRemotePeerWithId:(NSString*)peerId
{
    return self.busAttachment->ClearKeys([peerId UTF8String]);
}

- (QStatus)keyExpiration:(uint32_t*)timeout forRemotePeerId:(NSString*)peerId
{
    return self.busAttachment->GetKeyExpiration([peerId UTF8String], *timeout);
}

- (QStatus)setKeyExpiration:(uint32_t)timeout forRemotePeerId:(NSString*)peerId
{
    return self.busAttachment->SetKeyExpiration([peerId UTF8String], timeout);
}

- (NSString*)guidForPeerNamed:(NSString*)peerName
{
    qcc::String aGuid;
    QStatus status = self.busAttachment->GetPeerGUID([peerName UTF8String], aGuid);
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNBusAttachment::getGuidForPeerNamed:%@ failed. %@",peerName, [AJNStatus descriptionForStatusCode:status]);
    }
    return [NSString stringWithCString:aGuid.c_str() encoding:NSUTF8StringEncoding];
}

- (QStatus)setDaemonDebugLevel:(uint32_t)level forModule:(NSString*)module
{
    return self.busAttachment->SetDaemonDebug([module UTF8String], level);
}

+ (uint32_t)currentTimeStamp
{
    return BusAttachment::GetTimestamp();
}

@end
