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

#import "AJNObserver.h"
#import "AJNBusAttachment.h"
#import <alljoyn/Observer.h>
#import <alljoyn/BusAttachment.h>


using namespace ajn;

#pragma mark - (Internal) Key Object

////////////////////////////////////////////////////////////////////////////////
//
// Key Object
//

/** This object represents a key used by the cache to identify an associated managed proxy object
 *
 * The cache is a plain NSMutableDictionary, which requires the keys to implement NSCopying protocol.
 * The NSCache is not used here for the cache, because NSCache has automated revocation. 
 * (automated revocation is what whe want to avoid)
 */
@interface AJNObjectId : NSObject <NSCopying>
@property (nonatomic,strong) NSString* objectPath;
@property (nonatomic,strong) NSString* uniqueBusName;

-(instancetype)initWithObjectPath:(NSString *)objPath uniqueBusName:(NSString *)objName;
-(BOOL)isEqual:(id)objectId;
-(NSUInteger) hash;
-(BOOL)isValid;
-(NSString *)concatenate;
@end

@implementation AJNObjectId
-(instancetype)initWithObjectPath:(NSString *)objPath uniqueBusName:(NSString *)objName
{
    if ((nil == objPath) || (nil == objName)) {
        return nil;
    }
    self = [super init];
    if (self) {
        self.objectPath = objPath;
        self.uniqueBusName = objName;
    }
    return self;
}

- (id)copyWithZone:(NSZone *)zone
{
    AJNObjectId *copy = [[[self class] allocWithZone:zone] init];
    if (nil != copy) {
        copy.objectPath = [self.objectPath copyWithZone:zone];
        copy.uniqueBusName = [self.uniqueBusName copyWithZone:zone];
    }
    return copy;
}

-(BOOL)isEqual:(id)objectId
{
    return (([((AJNObjectId*)objectId).objectPath isEqualToString:self.objectPath]) &&
            ([((AJNObjectId*)objectId).uniqueBusName isEqualToString:self.uniqueBusName]));
}

- (NSUInteger) hash
{
    return [[self concatenate] hash];
}

-(BOOL)isValid
{
    return ((NO == [self.uniqueBusName isEqualToString:@""]) && (NO == [self.objectPath isEqualToString:@""]));
}

-(NSString *)concatenate
{
    return [NSString stringWithFormat:@"%@%@",self.uniqueBusName,self.objectPath];
}
@end

////////////////////////////////////////////////////////////////////////////////

#pragma mark Extension

@interface AJNObserver ()
@property (nonatomic, strong) NSOperationQueue* listenersTaskQueue;
@property (nonatomic, strong) NSMutableArray *listeners;
@property (nonatomic, strong) NSMutableDictionary *proxyCache;
@property (nonatomic, strong) NSLock *proxyCacheLock;
@property (nonatomic, strong) AJNBusAttachment *busAttachment;
@property (nonatomic, strong) Class proxyType;

- (void)discoveredObject:(AJNHandle)proxyObject;
- (void)lostObject:(AJNHandle)proxyObject;

@end

#pragma mark - Native Listener

namespace ajn {
    class NativeListener : public Observer::Listener
    {
    public:
        NativeListener(AJNObserver *objCObject):objC(objCObject) {}
        virtual ~NativeListener() {}

        virtual void ObjectDiscovered(ProxyBusObject& proxyObj) {
            [objC discoveredObject:&proxyObj];
        }

        virtual void ObjectLost(ProxyBusObject& proxyObj) {
            [objC lostObject:&proxyObj];
        }
    private:
        __weak AJNObserver *objC;
    };
}

#pragma mark -
@implementation AJNObserver
{
    NativeListener* observerListener;
}

#pragma mark Memory Management

- (instancetype)initWithProxyType:(Class)proxyType busAttachment:(AJNBusAttachment*)bus mandatoryInterfaces:(NSArray *)interfaces
{
    // Validate input
    if ((nil == proxyType) || (nil == bus) || (nil == interfaces) || (0 == [interfaces count])){
        NSLog(@"ERROR: Failed to create Observer due to invalid input. ");
        return nil;
    }
    // All interfaces should be of type NSString
    for (id iface in interfaces){
        if (NO == [iface isKindOfClass:[NSString class]]){
            NSLog(@"ERROR: Failed to create Observer due to invalid interface type. ");
            return nil;
        }
    }

    // Convert interfaces to c-array
    BusAttachment *nativeBusAttachment = static_cast<BusAttachment*>(bus.handle);
    NSUInteger count = [interfaces count];
    const char* ifaces[count];
    for (NSUInteger i=0; i<count; ++i){
        ifaces[i] = [[interfaces objectAtIndex:i]UTF8String];
    }

    self = [super init];
    if (self) {
        // Intialize internals
        self.busAttachment = bus;
        self.listenersTaskQueue = [[NSOperationQueue alloc]init];
        self.listenersTaskQueue.maxConcurrentOperationCount = 1;
        self.listeners = [[NSMutableArray alloc]init];
        self.proxyCache = [[NSMutableDictionary alloc]init];
        self.proxyCacheLock = [[NSLock alloc]init];
        self.proxyType = proxyType;

        // Create and setup native counterpart
        observerListener = new NativeListener(self);
        Observer* pObserver = new Observer(*nativeBusAttachment, ifaces, count);
        self.handle = pObserver;
        pObserver->RegisterListener(*(static_cast<ajn::Observer::Listener*>(observerListener)));
    }
    return self;
}

- (void)dealloc
{
    // Remove first the Native Listener to stop new callbacks
    Observer* pObserver = static_cast<Observer*>(self.handle);
    if (NULL != pObserver) {
        ajn::Observer::Listener* pListener = static_cast<ajn::Observer::Listener*>(observerListener);
        if (NULL != pListener) {
            pObserver->UnregisterListener(*pListener);
            delete pListener;
        }
    }

    // Stop all pending listener tasks
    [self.listenersTaskQueue cancelAllOperations];

    // Operations use weak reference to self, wait untill all pending tasks are cancelled and running tasks finished
    // Prevents weak self becoming nil during execution of a task.
    [self.listenersTaskQueue waitUntilAllOperationsAreFinished];
    [self.proxyCacheLock lock];
    [self.proxyCache removeAllObjects];
    [self.listeners removeAllObjects];
    [self.proxyCacheLock unlock];

    if (NULL != pObserver) {
        delete pObserver;
    }
    self.handle = NULL;
    self.busAttachment = nil;
}

#pragma mark - Public interfaces

- (void)registerObserverListener:(id<AJNObserverListener>)listener triggerOnExisting:(BOOL)triggerOnExisting
{
    if (nil != listener) {
        NSBlockOperation* blockOperation = [[NSBlockOperation alloc] init];
        __weak NSBlockOperation* weakblockOperation = blockOperation;
        __weak AJNObserver* weakSelf = self;

        // Register given listener
        [blockOperation addExecutionBlock: ^{
            if ((YES == [weakblockOperation isCancelled]) || (nil == weakSelf)) {
                return;
            }
            [weakSelf.listeners addObject:listener];
             
            if (YES == triggerOnExisting) {
                [weakSelf.proxyCacheLock lock];
                for (AJNObjectId* key in weakSelf.proxyCache) {
                    // Invoke callback on main queue
                    dispatch_async(dispatch_get_main_queue(), ^{
                        [listener didDiscoverObject:[weakSelf.proxyCache objectForKey:key] forObserver:weakSelf];
                    });
                }
                [weakSelf.proxyCacheLock unlock];
            }
        }];
        [self.listenersTaskQueue addOperation:blockOperation];
    }
}

- (void)unregisterObserverListener:(id<AJNObserverListener>)listener
{
    if (nil != listener) {

        NSBlockOperation* blockOperation = [[NSBlockOperation alloc] init];
        __weak NSBlockOperation* weakblockOperation = blockOperation;
        __weak AJNObserver* weakSelf = self;

        // Remove given listener
        [blockOperation addExecutionBlock: ^{
            if ((YES == [weakblockOperation isCancelled]) || (nil == weakSelf)) {
                return;
            }
            [weakSelf.listeners removeObjectAtIndex:[weakSelf.listeners indexOfObject:listener]];
        }];
        [self.listenersTaskQueue addOperation:blockOperation];
    }
}

- (void)unregisterAllObserverListeners
{
    NSBlockOperation* blockOperation = [[NSBlockOperation alloc] init];
    __weak NSBlockOperation* weakblockOperation = blockOperation;
    __weak AJNObserver* weakSelf = self;

    // Revome all
    [blockOperation addExecutionBlock: ^{
        if ((YES == [weakblockOperation isCancelled]) || (nil == weakSelf)) {
            return;
        }
        [weakSelf.listeners removeAllObjects];
    }];
    [self.listenersTaskQueue addOperation:blockOperation];
}

- (AJNProxyBusObject*)getProxyForUniqueName:(NSString *)uniqueName objectPath:(NSString *)path
{
    AJNProxyBusObject *proxy = nil;
    AJNObjectId * objId =[[AJNObjectId alloc]initWithObjectPath:path uniqueBusName:uniqueName];
    if (NO == [objId isValid]) {
        return nil;
    }

    [self.proxyCacheLock lock];
    proxy = [self.proxyCache objectForKey:objId];
    [self.proxyCacheLock unlock];
    return proxy;
}

- (AJNProxyBusObject *)getFirstProxy
{
    AJNProxyBusObject *proxy = nil;
    [self.proxyCacheLock lock];
    if (0 < [self.proxyCache count]){
        NSArray *keys = [self.proxyCache allKeys];
        keys = [keys sortedArrayUsingComparator:^(id a, id b) {
            return [[((AJNObjectId*)a) concatenate] caseInsensitiveCompare:[((AJNObjectId*)b) concatenate]];
        }];
        proxy = [self.proxyCache objectForKey:[keys objectAtIndex:0]];
    }
    [self.proxyCacheLock unlock];
    return proxy;
}

- (AJNProxyBusObject *)getProxyFollowing:(AJNProxyBusObject *)previousObject
{
    AJNProxyBusObject *proxy = nil;
    if ((nil == previousObject) || (NO == previousObject.isValid)){
        return proxy;
    }

    // Create Key from previous object
    AJNObjectId *objId =[[AJNObjectId alloc]initWithObjectPath:previousObject.path uniqueBusName:previousObject.serviceName];
    if (NO == [objId isValid]) {
        return proxy;
    }

    // Find position of the key in the list of keys and take the next one in line
    [self.proxyCacheLock lock];
    if (0 < [self.proxyCache count]){
        NSArray *keys = [self.proxyCache allKeys];
        keys = [keys sortedArrayUsingComparator:^(id obj1, id obj2) {
            return [[((AJNObjectId*)obj1) concatenate] caseInsensitiveCompare:[((AJNObjectId*)obj2) concatenate]];
        }];
        NSUInteger index = [keys indexOfObject:objId];
        if (NSNotFound != index) {
            ++index;
            // Lookup with next index
            if ([keys count] > index){
                proxy = [self.proxyCache objectForKey:[keys objectAtIndex:index]];
            }
        } else {
            // Object not found: get an isertion index (this will give the index of the next object)
            [keys indexOfObject:objId
                  inSortedRange:(NSRange){0, [keys count]}
                        options:NSBinarySearchingInsertionIndex
                usingComparator:^NSComparisonResult(id obj1, id obj2) {
                    return [[((AJNObjectId*)obj1) concatenate] caseInsensitiveCompare:[((AJNObjectId*)obj2) concatenate]];
                }];
            // Lookup with index
            if ([keys count] > index){
                proxy = [self.proxyCache objectForKey:[keys objectAtIndex:index]];
            }
        }
    }
    [self.proxyCacheLock unlock];
    return proxy;
}

#pragma mark - Internal interfaces

- (void)discoveredObject:(AJNHandle)proxyObject
{
    // Create OjbC counterpart
    AJNProxyBusObject *proxyObj = [[AJNProxyBusObject alloc] initWithBusAttachment:self.busAttachment usingProxyBusObject:proxyObject];

    // Create Key
    AJNObjectId *objId =[[AJNObjectId alloc]initWithObjectPath:[NSString stringWithUTF8String:(((ProxyBusObject*)proxyObject))->GetPath().c_str()]
                                                 uniqueBusName:[NSString stringWithUTF8String:(((ProxyBusObject*)proxyObject))->GetUniqueName().c_str()]];

    if (NO == [objId isValid]) {
        return;
    }

    NSBlockOperation* blockOperation = [[NSBlockOperation alloc] init];
    __weak NSBlockOperation* weakblockOperation = blockOperation;
    __weak AJNObserver* weakSelf = self;

    // Inform all listeners about newly discovered object
    [blockOperation addExecutionBlock: ^{
        if ((YES == [weakblockOperation isCancelled]) || (nil == weakSelf)) {
            return;
        }

        // Add to local cache
        [self.proxyCacheLock lock];
        [self.proxyCache setObject:proxyObj forKey:objId];
        [self.proxyCacheLock unlock];

        for (id<AJNObserverListener> listener in weakSelf.listeners) {
            if ([weakblockOperation isCancelled]) {
                break;
            }

            // Invoke callback on main queue
            dispatch_async(dispatch_get_main_queue(), ^{
                [listener didDiscoverObject:proxyObj forObserver:weakSelf];
            });
        }
    }];
    [self.listenersTaskQueue addOperation:blockOperation];
}

- (void)lostObject:(AJNHandle)proxyObject
{
    // Create Key
    AJNObjectId *objId =[[AJNObjectId alloc]initWithObjectPath:[NSString stringWithUTF8String:(((ProxyBusObject*)proxyObject))->GetPath().c_str()]
                                                 uniqueBusName:[NSString stringWithUTF8String:(((ProxyBusObject*)proxyObject))->GetUniqueName().c_str()]];

    if (NO == [objId isValid]) {
        return;
    }

    NSBlockOperation* blockOperation = [[NSBlockOperation alloc] init];
    __weak NSBlockOperation* weakblockOperation = blockOperation;
    __weak AJNObserver* weakSelf = self;

    // Inform all listeners about object removal
    [blockOperation addExecutionBlock: ^{
        if ((YES == [weakblockOperation isCancelled]) || (nil == weakSelf)) {
            return;
        }
        // Remove from local cache
        [self.proxyCacheLock lock];
        AJNProxyBusObject* proxyObj = [self.proxyCache objectForKey:objId];
        if (nil == proxyObj) {
            [self.proxyCacheLock unlock];
            return;
        }
        [self.proxyCache removeObjectForKey:objId];
        [self.proxyCacheLock unlock];

        for (id<AJNObserverListener> listener in weakSelf.listeners) {
            if ([weakblockOperation isCancelled]) {
                break;
            }
            // Invoke callback on main queue
            dispatch_async(dispatch_get_main_queue(), ^{
                [listener didLoseObject:proxyObj forObserver:weakSelf];
            });
        }
    }];
    [self.listenersTaskQueue addOperation:blockOperation];
}

@end
