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
 * (automated revocation is what we want to avoid)
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
    // Stop all pending listener tasks
    [self.listenersTaskQueue cancelAllOperations];

    // Operations use weak reference to self, wait untill all pending tasks are cancelled and running tasks finished
    // Prevents weak self becoming nil during execution of a task.
    [self.listenersTaskQueue waitUntilAllOperationsAreFinished];
    [self.proxyCacheLock lock];
    [self.proxyCache removeAllObjects];
    [self.listeners removeAllObjects];
    [self.proxyCacheLock unlock];

    // Move the actual destruction of the Observer out of this context, because there is a
    // potential deadlock lurking here:
    // ARC may find that the last valid reference to an AJNObserver exists in its NativeListener
    // (the timing needs to be exactly right, but we've seen this happen with the CreateDelete
    // unit test). That would cause this dealloc to be called from the closing brace of
    // NativeListener::ObjectDiscovered, which leads to a deadlock as you're trying to unregister
    // the listener from within that listener itself.
    Observer* pObserver = static_cast<Observer*>(self.handle);
    NativeListener* blockListener = observerListener;
    dispatch_async(dispatch_get_main_queue(), ^{
        if (pObserver) {
            pObserver->UnregisterAllListeners();
            delete pObserver;
        }
        if (blockListener) {
            delete blockListener;
        }
    });

    self.handle = NULL;
    self.busAttachment = nil;
    observerListener = NULL;
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
            /* Take a strong reference to the AJNObserver to make sure it
             * doesn't get ARCed between taking and releasing the proxyCacheLock.
             */
            AJNObserver* strongSelf = weakSelf;
            if ((YES == [weakblockOperation isCancelled]) || (nil == strongSelf)) {
                return;
            }
            [strongSelf.listeners addObject:listener];

            if (YES == triggerOnExisting) {
                [strongSelf.proxyCacheLock lock];
                for (AJNObjectId* key in weakSelf.proxyCache) {
                    // Invoke callback on main queue
                    __weak id<AJNObserverListener> weakListener = listener;
                    AJNProxyBusObject* proxy = [strongSelf.proxyCache objectForKey:key];
                    dispatch_async(dispatch_get_main_queue(), ^{
                        /* Only invoke the callback if the observer and listener still exist.
                         * Take strong references to avoid parallel reclamation by ARC.
                         */
                        AJNObserver* observer = weakSelf;
                        if (nil != observer) {
                            [weakListener didDiscoverObject:proxy forObserver:observer];
                        }
                    });
                }
                [strongSelf.proxyCacheLock unlock];
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
    AJNProxyBusObject *proxyObj = [[self.proxyType alloc] initWithBusAttachment:self.busAttachment usingProxyBusObject:proxyObject];
    NSAssert(nil != proxyObj, @"proxyObj allocation failed");

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
        AJNObserver* strongSelf = weakSelf;
        if ((YES == [weakblockOperation isCancelled]) || (nil == strongSelf)) {
            return;
        }

        // Add to local cache
        [strongSelf.proxyCacheLock lock];
        [strongSelf.proxyCache setObject:proxyObj forKey:objId];
        [strongSelf.proxyCacheLock unlock];

        for (id<AJNObserverListener> listener in strongSelf.listeners) {
            if ([weakblockOperation isCancelled]) {
                break;
            }

            // Invoke callback on main queue
            __weak id<AJNObserverListener> weakListener = listener;
            dispatch_async(dispatch_get_main_queue(), ^{
                /* Only invoke the callback if the observer and listener still exist.
                 * Take strong references to avoid parallel reclamation by ARC. */
                AJNObserver* observer = weakSelf;
                if (nil != observer) {
                    [weakListener didDiscoverObject:proxyObj forObserver:observer];
                }
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
        AJNObserver* strongSelf = weakSelf;
        if ((YES == [weakblockOperation isCancelled]) || (nil == strongSelf)) {
            return;
        }

        // Remove from local cache
        [strongSelf.proxyCacheLock lock];
        AJNProxyBusObject* proxyObj = [strongSelf.proxyCache objectForKey:objId];
        [strongSelf.proxyCache removeObjectForKey:objId];
        [strongSelf.proxyCacheLock unlock];

        if (nil == proxyObj) {
            return;
        }

        for (id<AJNObserverListener> listener in strongSelf.listeners) {
            if ([weakblockOperation isCancelled]) {
                break;
            }

            // Invoke callback on main queue
            __weak id<AJNObserverListener> weakListener = listener;
            dispatch_async(dispatch_get_main_queue(), ^{
                /* Only invoke the callback if the observer and listener still exist.
                 * Take strong references to avoid parallel reclamation by ARC. */
                AJNObserver* observer = weakSelf;
                if (nil != observer) {
                    [weakListener didLoseObject:proxyObj forObserver:observer];
                }
            });
        }
    }];
    [self.listenersTaskQueue addOperation:blockOperation];
}

@end
