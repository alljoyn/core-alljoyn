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

#import "ObserverTests.h"
#import "AJNBusAttachment.h"
#import "AJNObserver.h"
#import "AJNObserverListener.h"
#import "ObserverTestObjects.h"
#import "AJNAboutObject.h"
#import "AJNInit.h"
#import <sys/time.h>

static NSString * const kObserverTestsAdvertisedName = @"org.alljoyn.observer.tests.AReallyNiftyNameThatNoOneWillUse";
static NSString * const kObserverTestsInterfaceNameA = @"org.test.a";
static NSString * const kObserverTestsInterfaceNameB = @"org.test.b";
static NSString * const kObserverTestsInterfaceMethodA = @"IdentifyA";
static NSString * const kObserverTestsInterfaceMethodB = @"IdentifyB";
static NSString * const kObserverTestsObjectA = @"justA";
static NSString * const kObserverTestsObjectB = @"justB";
static NSString * const kObserverTestsObjectBoth = @"both";
static NSString * const kObserverTestsObjectBoth2 = @"both2";
const NSTimeInterval kObserverTestsWaitTimeBeforeFailure = 10.0;
const NSInteger kObserverTestsServicePort = 42;
static NSString * const kPathPrefix = @"/test/";

#define MAX_WAIT_MS 3000


////////////////////////////////////////////////////////////////////////////////
//
//               ObjectState
//
////////////////////////////////////////////////////////////////////////////////
@interface ObjectState : NSObject
@property (nonatomic,strong) AJNBusObject* obj;
@property (nonatomic,assign) BOOL registered;

-(instancetype)initWithObject:(AJNBusObject *)testObj registered:(BOOL)registered;
@end

@implementation ObjectState

-(instancetype)initWithObject:(AJNBusObject *)testObj registered:(BOOL)registered
{
    self =[super init];
    if (self) {
        self.obj = testObj;
        self.registered = registered;
    }
    return self;
}

@end

////////////////////////////////////////////////////////////////////////////////
//
//               Participant
//
////////////////////////////////////////////////////////////////////////////////

@interface Participant : NSObject <AJNSessionListener, AJNSessionPortListener, AJNAboutDataListener>

@property (nonatomic, strong) AJNBusAttachment *bus;
@property (nonatomic, strong) AJNAboutObject *aboutObj;
@property (nonatomic, strong) NSString *uniqueBusName;
@property (nonatomic, strong) NSMutableDictionary *aboutData;
@property (nonatomic, strong) NSMutableDictionary *ObjectMap;
@property (nonatomic, strong) NSMutableDictionary *hostedSessionMap;
@property (nonatomic, strong) NSLock* hostedSessionMapLock;
@property (nonatomic,assign) BOOL acceptSessions;
@property (nonatomic,assign) BOOL registerInterfaces;

-(void)createObjectWithName:(NSString*)name;
-(void)registerObjectWithName:(NSString*)name;
-(void)unregisterObjectWithName:(NSString*)name;
-(void)closeSessionForParticipant:(Participant*)joiner;
-(BOOL)hasSessionWith:(Participant*)joiner;

@end

@implementation Participant

-(instancetype)initWithInterfaceRegistration:(BOOL)registerInterfaces
{
    self = [super init];
    if (self){
        self.aboutData = [[NSMutableDictionary alloc] initWithCapacity:16];
        self.ObjectMap = [[NSMutableDictionary alloc] init];
        self.hostedSessionMap = [[NSMutableDictionary alloc] init];
        self.hostedSessionMapLock = [[NSLock alloc]init];
        self.acceptSessions = YES;
        self.registerInterfaces = registerInterfaces;

        //--------------------------
        // Bus & session setup
        //--------------------------
        self.bus = [[AJNBusAttachment alloc] initWithApplicationName:@"Participant" allowRemoteMessages:YES];
        QStatus status = [self.bus start];
        NSAssert(status == ER_OK, @"Bus failed to start.");
        status = [self.bus connectWithArguments:@"null:"];
        NSAssert(status == ER_OK, @"Participant connection to bus via null transport failed.");

        AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages
                                                                        supportsMultipoint:YES
                                                                                 proximity:kAJNProximityAny
                                                                             transportMask:kAJNTransportMaskAny];

        status = [self.bus bindSessionOnPort:kObserverTestsServicePort
                                 withOptions:sessionOptions
                                withDelegate:self];

        NSAssert(status == ER_OK, ([NSString stringWithFormat:@"Bind session on port %ld failed.", (long)kObserverTestsServicePort]));

        self.uniqueBusName = self.bus.uniqueName;

        //--------------------------
        // Create Interfaces
        //--------------------------
        if (YES == registerInterfaces) {
            AJNInterfaceDescription *iface = [self.bus createInterfaceWithName:kObserverTestsInterfaceNameA
                                                                enableSecurity:NO];
            NSAssert(nil != iface, @"Bus failed to create interface.");
            status = [iface addMethodWithName:kObserverTestsInterfaceMethodA
                               inputSignature:@""
                              outputSignature:@"ss"
                                argumentNames:[NSArray arrayWithObjects:@"busname",@"path",nil]];
            NSAssert(status == ER_OK, @"Interface description failed to add method to interface.");
            [iface activate];

            iface = nil;
            iface = [self.bus createInterfaceWithName:kObserverTestsInterfaceNameB
                                       enableSecurity:NO];
            NSAssert(nil != iface, @"Bus failed to create interface.");
            status = [iface addMethodWithName:kObserverTestsInterfaceMethodB
                               inputSignature:@""
                              outputSignature:@"ss"
                                argumentNames:[NSArray arrayWithObjects:@"busname",@"path",nil]];
            NSAssert(status == ER_OK, @"Interface description failed to add method to interface.");
            [iface activate];
            iface = nil;
        }

        //--------------------------
        // Create About data
        //--------------------------
        AJNMessageArgument *appID = [[AJNMessageArgument alloc] init];
        uint8_t originalAppId[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
        [appID setValue:@"ay", sizeof(originalAppId) / sizeof(originalAppId[0]), originalAppId];
        [appID stabilize];
        [self.aboutData setValue:appID forKey:@"AppId"];

        AJNMessageArgument *defaultLang = [[AJNMessageArgument alloc] init];
        [defaultLang setValue:@"s", "en"];
        [defaultLang stabilize];
        [self.aboutData setValue:defaultLang forKey:@"DefaultLanguage"];

        AJNMessageArgument *deviceName = [[AJNMessageArgument alloc] init];
        [deviceName setValue:@"s", "Device Name"];
        [deviceName stabilize];
        [self.aboutData setValue:deviceName forKey:@"DeviceName"];

        AJNMessageArgument *deviceId = [[AJNMessageArgument alloc] init];
        [deviceId setValue:@"s", "avec-awe1213-1234559xvc123"];
        [deviceId stabilize];
        [self.aboutData setValue:deviceId forKey:@"DeviceId"];

        AJNMessageArgument *appName = [[AJNMessageArgument alloc] init];
        [appName setValue:@"s", "App Name"];
        [appName stabilize];
        [self.aboutData setValue:appName forKey:@"AppName"];

        AJNMessageArgument *manufacturer = [[AJNMessageArgument alloc] init];
        [manufacturer setValue:@"s", "Manufacturer"];
        [manufacturer stabilize];
        [self.aboutData setValue:manufacturer forKey:@"Manufacturer"];

        AJNMessageArgument *modelNo = [[AJNMessageArgument alloc] init];
        [modelNo setValue:@"s", "ModelNo"];
        [modelNo stabilize];
        [self.aboutData setValue:modelNo forKey:@"ModelNumber"];

        AJNMessageArgument *supportedLang = [[AJNMessageArgument alloc] init];
        const char *supportedLangs[] = {"en"};
        [supportedLang setValue:@"as", 1, supportedLangs];
        [supportedLang stabilize];
        [self.aboutData setValue:supportedLang forKey:@"SupportedLanguages"];

        AJNMessageArgument *description = [[AJNMessageArgument alloc] init];
        [description setValue:@"s", "Description"];
        [description stabilize];
        [self.aboutData setValue:description forKey:@"Description"];

        AJNMessageArgument *dateOfManufacture = [[AJNMessageArgument alloc] init];
        [dateOfManufacture setValue:@"s", "1-1-2014"];
        [dateOfManufacture stabilize];
        [self.aboutData setValue:dateOfManufacture forKey:@"DateOfManufacture"];

        AJNMessageArgument *softwareVersion = [[AJNMessageArgument alloc] init];
        [softwareVersion setValue:@"s", "1.0"];
        [softwareVersion stabilize];
        [self.aboutData setValue:softwareVersion forKey:@"SoftwareVersion"];

        AJNMessageArgument *ajSoftwareVersion = [[AJNMessageArgument alloc] init];
        [ajSoftwareVersion setValue:@"s", "15.04"];
        [ajSoftwareVersion stabilize];
        [self.aboutData setValue:ajSoftwareVersion forKey:@"AJSoftwareVersion"];

        AJNMessageArgument *hwSoftwareVersion = [[AJNMessageArgument alloc] init];
        [hwSoftwareVersion setValue:@"s", "15.04"];
        [hwSoftwareVersion stabilize];
        [self.aboutData setValue:hwSoftwareVersion forKey:@"HardwareVersion"];

        AJNMessageArgument *supportURL = [[AJNMessageArgument alloc] init];
        [supportURL setValue:@"s", "some.random.url"];
        [supportURL stabilize];
        [self.aboutData setValue:supportURL forKey:@"SupportUrl"];

        //--------------------------
        // Create & Announce About data
        //--------------------------
        self.aboutObj = [[AJNAboutObject alloc] initWithBusAttachment:self.bus withAnnounceFlag:ANNOUNCED];
        [self.aboutObj announceForSessionPort:kObserverTestsServicePort withAboutDataListener:self];
    }
    return self;
}

-(void)dealloc
{
    [self.ObjectMap enumerateKeysAndObjectsUsingBlock:^(id key, id obj, BOOL *stop) {
        ObjectState *objState = (ObjectState *)obj;
        if (YES == objState.registered) {
            [self.bus unregisterBusObject:objState.obj];
        }
        *stop = NO;
    }];

    if (YES == self.registerInterfaces) {
        [self.bus deleteInterfaceWithName:kObserverTestsInterfaceNameA];
        [self.bus deleteInterfaceWithName:kObserverTestsInterfaceNameB];
    }

    QStatus status = [self.bus disconnectWithArguments:@"null:"];
    NSAssert(status == ER_OK, @"Failed to disconnect from bus via null transport.");
    status = [self.bus stop];
    NSAssert(status == ER_OK, @"Bus failed to stop.");
    status = [self.bus waitUntilStopCompleted];
    NSAssert(status == ER_OK, @"Bus failed to complete stop action.");

    self.bus = nil;
}

#pragma mark - Public interfaces

-(void)createObjectWithName:(NSString *)name
{
    NSString *path = [NSString stringWithFormat:@"%@%@",kPathPrefix,name];
    AJNBusObject* obj = nil;

    if (YES == [name isEqualToString:kObserverTestsObjectA]){
        obj = [[TestJustA alloc]initWithBusAttachment:self.bus onPath:path];
    } else if (YES == [name isEqualToString:kObserverTestsObjectB]) {
        obj = [[TestJustB alloc]initWithBusAttachment:self.bus onPath:path];
    } else {
        obj = [[TestBoth alloc]initWithBusAttachment:self.bus onPath:path];
    }

    ObjectState *objState = [[ObjectState alloc]initWithObject:obj registered:NO];
    [self.ObjectMap setObject:objState forKey:name];
}

-(void)registerObjectWithName:(NSString *)name
{
    ObjectState *objState = [self.ObjectMap objectForKey:name];
    NSAssert(nil != objState,@"registerObjectWithName: %@, No such object.",name);
    NSAssert(NO == objState.registered, @"Object with name: %@, already on bus.", name);
    QStatus status = [self.bus registerBusObject:objState.obj];
    NSAssert(status == ER_OK, @"Failed to register object %@ on bus.", name);
    objState.registered = YES;
    [self.aboutObj announceForSessionPort:kObserverTestsServicePort withAboutDataListener:self];
}

-(void)unregisterObjectWithName:(NSString *)name
{
    ObjectState *objState = [self.ObjectMap objectForKey:name];
    NSAssert(nil != objState, @"%@: No such object.", name);
    NSAssert(objState.registered, @"%@: Object not on bus.", name);
    [self.bus unregisterBusObject:objState.obj];
    objState.registered = NO;
    [self.aboutObj announceForSessionPort:kObserverTestsServicePort withAboutDataListener:self];
}

-(void)closeSessionForParticipant:(Participant *)joiner
{
    [self.hostedSessionMapLock lock];
    [self.hostedSessionMap enumerateKeysAndObjectsUsingBlock:^(id key, id obj, BOOL *stop) {
        if (YES == [joiner.uniqueBusName isEqualToString:(NSString *)key]){
            [self.bus leaveHostedSession:[((NSNumber *)obj) unsignedIntValue]];
            [self.hostedSessionMap removeObjectForKey:key];
            *stop = YES;
        } else {
            *stop = NO;
        }
    }];
    [self.hostedSessionMapLock unlock];
}

-(BOOL)hasSessionWith:(Participant*)joiner
{
    [self.hostedSessionMapLock lock];
    __block BOOL result = NO;
    [self.hostedSessionMap enumerateKeysAndObjectsUsingBlock:^(id key, id obj, BOOL *stop) {
        if (YES == [joiner.uniqueBusName isEqualToString:(NSString *)key]){
            *stop = YES;
            result = YES;
        } else {
            *stop = NO;
        }
    }];
    [self.hostedSessionMapLock unlock];
    return result;
}

#pragma mark - AJNSessionListener protocol

- (void)sessionWasLost:(AJNSessionId)sessionId forReason:(AJNSessionLostReason)reason
{
    [self.hostedSessionMapLock lock];
    [self.hostedSessionMap enumerateKeysAndObjectsUsingBlock:^(id key, id obj, BOOL *stop) {
        if (sessionId == [((NSNumber *)obj) unsignedIntValue]){
            [self.hostedSessionMap removeObjectForKey:key];
            *stop = YES;
        } else {
            *stop = NO;
        }
    }];
    [self.hostedSessionMapLock unlock];
}

#pragma mark - AJNSessionPortListener protocol

- (BOOL)shouldAcceptSessionJoinerNamed:(NSString *)joiner onSessionPort:(AJNSessionPort)sessionPort withSessionOptions:(AJNSessionOptions *)options
{
    NSLog(@"AJNSessionPortListener::shouldAcceptSessionJoinerNamed:%@ onSessionPort:%u withSessionOptions:", joiner, sessionPort);
    return self.acceptSessions;
}

- (void)didJoin:(NSString *)joiner inSessionWithId:(AJNSessionId)sessionId onSessionPort:(AJNSessionPort)sessionPort
{
    NSLog(@"AJNSessionPortListener::didJoin:%@ inSessionWithId:%u onSessionPort:%u withSessionOptions:", joiner, sessionId, sessionPort);

    [self.hostedSessionMapLock lock];
    [self.hostedSessionMap setObject:[NSNumber numberWithUnsignedInt:sessionId] forKey:joiner];
    [self.bus bindHostedSessionListener:self toSession:sessionId];
    [self.hostedSessionMapLock unlock];
}

#pragma mark - AJNAboutDataListener protocol

- (QStatus)getAboutDataForLanguage:(NSString *)language usingDictionary:(NSMutableDictionary **)aboutData
{
    // re-use the defaults
    return [self getDefaultAnnounceData:aboutData];
}

-(QStatus)getDefaultAnnounceData:(NSMutableDictionary **)aboutData
{
    *aboutData = self.aboutData;
    return ER_OK;
}

@end

////////////////////////////////////////////////////////////////////////////////
//
//               ObserverListener
//
////////////////////////////////////////////////////////////////////////////////

@interface ObserverListener : NSObject <AJNObserverListener>
@property (nonatomic,weak) AJNBusAttachment *bus;
@property (nonatomic,strong) NSMutableArray *proxies;
@property (nonatomic,assign) NSUInteger counter;
@property (nonatomic,assign) BOOL tolerateAlreadyDiscoveredObjects;

-(instancetype)initWithBusAttachment:(AJNBusAttachment *)bus;
-(void)expectInvocations:(NSUInteger)newCounter;
-(void)checkReentrancyForProxy:(AJNProxyBusObject *)proxy;
-(void)reset;
@end

@implementation ObserverListener

-(instancetype)initWithBusAttachment:(AJNBusAttachment *)bus
{
    self = [super init];
    if (self){
        self.bus = bus;
        self.proxies = [[NSMutableArray alloc]init];
        self.counter = 0;
        self.tolerateAlreadyDiscoveredObjects = NO;
    }
    return self;
}

-(void)dealloc
{
    self.proxies = nil;
}

-(void) reset
{
    self.counter = 0;
    [self.proxies removeAllObjects];
}

-(void)expectInvocations:(NSUInteger)newCounter
{
    NSAssert(0 == self.counter, @"In the previous test case, the listener was triggered an invalid number of times.");
    self.counter = newCounter;
}

-(void)checkReentrancyForProxy:(AJNProxyBusObject *)proxy
{
    NSString *busname;
    NSString *path;
    if (YES == [proxy respondsToSelector:@selector(identifyAWithBusname:andPath:)]) {
        if ( YES == [proxy respondsToSelector:@selector(identifyBWithBusname:andPath:)]) {
            TestBothProxy *proxyBoth = (TestBothProxy *)proxy;
            [proxyBoth identifyAWithBusname:&busname andPath:&path];
        } else {
            TestJustAProxy *proxyA = (TestJustAProxy *)proxy;
            [proxyA identifyAWithBusname:&busname andPath:&path];
        }
    } else if (YES == [proxy respondsToSelector:@selector(identifyBWithBusname:andPath:)]) {
        TestJustBProxy *proxyB = (TestJustBProxy *)proxy;
        [proxyB identifyBWithBusname:&busname andPath:&path];
    } else {
        // Invalid object
        NSAssert(0==1,@"Invalid Proxy object.");
    }
    NSAssert(YES == [path isEqualToString:proxy.path] , @"Proxy: Method reply does not contain correct object path");
}

#pragma mark - AJNObserverListener protocol

-(void)didDiscoverObject:(AJNProxyBusObject *)obj forObserver:(AJNObserver *)observer
{
    NSAssert(nil != obj, @"Discovered object: Invalid Proxy");
    if (NO == self.tolerateAlreadyDiscoveredObjects) {
        BOOL found = NO;
        // AJNProxyBusObject does not support hash and isEqual: => we need to iterate ourselves
        for (NSUInteger idx = 0; idx < [self.proxies count]; ++idx) {
            AJNProxyBusObject* pproxy = [self.proxies objectAtIndex:idx];
            if ([obj.path isEqualToString:pproxy.path] && [obj.uniqueName isEqualToString:pproxy.uniqueName]) {
                found = YES;
                break;
            }
        }
        NSAssert(!found, @"Discovering an already-discovered object.");
    }
    [self.proxies addObject:obj];
    [self checkReentrancyForProxy:obj];
    --self.counter;
}

-(void)didLoseObject:(AJNProxyBusObject *)obj forObserver:(AJNObserver *)observer;
{
    NSAssert(nil != obj, @"Lost object: Invalid Proxy");
    // AJNProxyBusObject does not support hash and isEqual:
    BOOL found = NO;
    for (NSUInteger idx = 0; idx < [self.proxies count]; ++idx) {
        AJNProxyBusObject* pproxy = [self.proxies objectAtIndex:idx];
        if ([obj.path isEqualToString:pproxy.path] && [obj.uniqueName isEqualToString:pproxy.uniqueName]) {
            [self.proxies removeObjectAtIndex:idx];
            found = YES;
            --self.counter;
            break;
        }
    }
    NSAssert(found, @"Lost a not-discovered object.");
}

@end

////////////////////////////////////////////////////////////////////////////////
//
//               Actual unit test cases
//
////////////////////////////////////////////////////////////////////////////////
@interface ObserverTests ()

typedef BOOL (^verifyObjects)();

-(NSUInteger)countProxies:(AJNObserver *) observer;
-(BOOL)waitForBlock:(verifyObjects)checkResult msToWait:(NSTimeInterval)msToWait;
-(void)simpleScenario:(Participant*)provider consumer:(Participant*) consumer;
@end

@implementation ObserverTests

- (void)setUp
{
    [super setUp];
    // Set-up code here. Executed before each test case is run.
    //
}

- (void)tearDown
{
    // Tear-down code here. Executed after each test case is run.
    //
    [super tearDown];

    // Wait 500ms to let object cleanup stabalize
    struct timeval time;
    gettimeofday(&time, NULL);
    long now = (time.tv_sec * 1000) + (time.tv_usec / 1000);
    long final = now + 500;
    [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate dateWithTimeIntervalSince1970:(final/1000)]];
}

+(void)setUp
{
    [AJNInit alljoynInit];
    [AJNInit alljoynRouterInit];
}

+(void)tearDown
{
    [AJNInit alljoynRouterShutdown];
    [AJNInit alljoynShutdown];
}

-(NSUInteger)countProxies:(AJNObserver *) observer
{
    NSUInteger count = 0;
    AJNProxyBusObject *proxy = [observer getFirstProxy];
    while (nil != proxy) {
        ++count;
        proxy = [observer getProxyFollowing:proxy];
    }
    return count;
}

-(BOOL)waitForBlock:(verifyObjects)checkResult msToWait:(NSTimeInterval)msToWait
{
    struct timeval time;
    gettimeofday(&time, NULL);
    long now = (time.tv_sec * 1000) + (time.tv_usec / 1000);
    long final = now + msToWait;
    BOOL result = checkResult();
    while ((NO == result) && (now < final)) {
        // Process other tasks pending on the main queue before proceeding current test
        [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate dateWithTimeIntervalSince1970:(final/1000)]];
        result = checkResult();
        gettimeofday(&time, NULL);
        now = (time.tv_sec * 1000) + (time.tv_usec / 1000);
    }
    return result;
}

-(void)simpleScenario:(Participant*)provider consumer:(Participant*) consumer
{
    // Create Provider objects
    [provider createObjectWithName:kObserverTestsObjectA];
    [provider createObjectWithName:kObserverTestsObjectB];
    [provider createObjectWithName:kObserverTestsObjectBoth];

    // Create Listeners for the observers
    ObserverListener *listenerA = [[ObserverListener alloc]initWithBusAttachment:consumer.bus];
    ObserverListener *listenerB = [[ObserverListener alloc]initWithBusAttachment:consumer.bus];
    ObserverListener *listenerAB = [[ObserverListener alloc]initWithBusAttachment:consumer.bus];

    // Create 3 observers and register listeners
    AJNObserver *obsA = [[AJNObserver alloc]initWithProxyType:[TestJustAProxy class]
                                                busAttachment:consumer.bus
                                          mandatoryInterfaces:[NSArray arrayWithObject:kObserverTestsInterfaceNameA]];
    [obsA registerObserverListener:listenerA triggerOnExisting:YES];
    AJNObserver *obsB = [[AJNObserver alloc]initWithProxyType:[TestJustBProxy class]
                                                busAttachment:consumer.bus
                                          mandatoryInterfaces:[NSArray arrayWithObject:kObserverTestsInterfaceNameB]];
    [obsB registerObserverListener:listenerB triggerOnExisting:YES];
    AJNObserver *obsAB = [[AJNObserver alloc]initWithProxyType:[TestBothProxy class]
                                                 busAttachment:consumer.bus
                                           mandatoryInterfaces:[NSArray arrayWithObjects:kObserverTestsInterfaceNameA,kObserverTestsInterfaceNameB,nil]];
    [obsAB registerObserverListener:listenerAB triggerOnExisting:YES];

    // Get the object counts of the listeners
    verifyObjects allListeners = ^BOOL() {
        return ((0 == listenerA.counter) &&
                (0 == listenerB.counter) &&
                (0 == listenerAB.counter));
    };

    // 1. Provider: publish all objects
    [listenerA expectInvocations:2];
    [listenerB expectInvocations:2];
    [listenerAB expectInvocations:1];

    [provider registerObjectWithName:kObserverTestsObjectA];
    [provider registerObjectWithName:kObserverTestsObjectB];
    [provider registerObjectWithName:kObserverTestsObjectBoth];
    STAssertTrue(YES == [self waitForBlock:allListeners msToWait:MAX_WAIT_MS], @"Not all objects were discovered correctly");

    // 2. Test Observer::Get() and the proxy creation functionality
    AJNProxyBusObject *proxy = [obsA getProxyForUniqueName:provider.bus.uniqueName
                                                objectPath:[NSString stringWithFormat:@"%@%@",kPathPrefix,kObserverTestsObjectA]];
    STAssertNotNil(proxy, @"Proxy object for justA not found");
    NSArray *interfaces = [proxy interfaces];
    STAssertTrue(2 == [interfaces count], @"Not getting correct number of interfaces from proxy justA");
    proxy = [obsA getProxyForUniqueName:provider.bus.uniqueName
                             objectPath:[NSString stringWithFormat:@"%@%@",kPathPrefix,kObserverTestsObjectBoth]];
    STAssertNotNil(proxy, @"Proxy object for both not found");
    interfaces = [proxy interfaces];
    STAssertTrue(3 == [interfaces count], @"Not getting correct number of interfaces from proxy both");

    // Verify that we can indeed perform method calls
    AJNMessage *reply;

    QStatus status = [proxy callMethodWithName:kObserverTestsInterfaceMethodA
                           onInterfaceWithName:kObserverTestsInterfaceNameA
                                 withArguments:nil
                                   methodReply:&reply];
    STAssertTrue(ER_OK == status, @"Message call failed for interface A on proxy object both");
    NSArray *arguments = reply.arguments;
    STAssertTrue(2 == [arguments count], @"Method reply does not contain correct number of arguments");

    AJNMessageArgument *arg = [arguments objectAtIndex:0];
    AJNMessageArgument *arg2 = [arguments objectAtIndex:1];
    const char *objectPath;
    const char *busname;
    status = [arg value:@"s",&busname];
    STAssertTrue(ER_OK == status, @"Could not extract unique bus name from argument ");
    NSString * uniqueBusName = [NSString stringWithUTF8String:busname];
    status = [arg2 value:@"s",&objectPath];
    STAssertTrue(ER_OK == status, @"Could not extract object path from argument ");
    NSString *objPath = [NSString stringWithUTF8String:objectPath];
    STAssertTrue(YES == [provider.bus.uniqueName isEqualToString:uniqueBusName], @"Method reply does not contain correct unique bus name");

    NSString *origObjPath = [NSString stringWithFormat:@"%@%@",kPathPrefix,kObserverTestsObjectBoth];
    STAssertTrue(YES == [objPath isEqualToString:origObjPath] , @"Method reply does not contain correct path");

    // 3. Remove "justA" from the bus
    [listenerA expectInvocations:1];
    [listenerB expectInvocations:0];
    [listenerAB expectInvocations:0];

    [provider unregisterObjectWithName:kObserverTestsObjectA];
    STAssertTrue(YES == [self waitForBlock:allListeners msToWait:MAX_WAIT_MS], @"justA not detected as lost correctly");

    // 4. Remove "both" from the bus
    [listenerA expectInvocations:1];
    [listenerB expectInvocations:1];
    [listenerAB expectInvocations:1];

    [provider unregisterObjectWithName:kObserverTestsObjectBoth];
    STAssertTrue(YES == [self waitForBlock:allListeners msToWait:MAX_WAIT_MS], @"Object both not detected as lost correctly");

    // Count the number of proxies left in the Observers
    // here should be 0 in A, 1 in B, 0 in AB
    STAssertTrue(0 == [self countProxies:obsA], @"Observer A has still proxies in the cache");
    STAssertTrue(1 == [self countProxies:obsB], @"Observer B schould have only 1 proxy in the cache");
    STAssertTrue(0 == [self countProxies:obsAB], @"Observer AB has still proxies in the cache");

    // Remove All listeners
    [obsA unregisterAllObserverListeners];
    [obsB unregisterAllObserverListeners];
    [obsAB unregisterObserverListener:listenerAB];

    // remove "justB" and reinstate the other objects
    [listenerA expectInvocations:0];
    [listenerB expectInvocations:0];
    [listenerAB expectInvocations:0];

    [provider unregisterObjectWithName:kObserverTestsObjectB];
    [provider registerObjectWithName:kObserverTestsObjectA];
    [provider registerObjectWithName:kObserverTestsObjectBoth];

    // Busy-wait for a second at most
    verifyObjects numberOfProxies = ^BOOL() {
        return ((2 == [self countProxies:obsA]) &&
                (1 == [self countProxies:obsB]) &&
                (1 == [self countProxies:obsAB]));
    };
    STAssertTrue(YES == [self waitForBlock:numberOfProxies msToWait:1000], @"incorrect number of proxies detected");

    // Reinstate listeners & test triggerOnExisting functionality
    [listenerA reset];
    [listenerA expectInvocations:2];
    [listenerB reset];
    [listenerB expectInvocations:1];
    [listenerAB reset];
    [listenerAB expectInvocations:1];

    [obsA registerObserverListener:listenerA triggerOnExisting:YES];
    [obsB registerObserverListener:listenerB triggerOnExisting:YES];
    [obsAB registerObserverListener:listenerAB triggerOnExisting:YES];
    STAssertTrue(YES == [self waitForBlock:allListeners msToWait:MAX_WAIT_MS], @"Not all objects where discovered correctly");

    // 5. Test multiple listeners for same observer
    ObserverListener *listenerB2 = [[ObserverListener alloc]initWithBusAttachment:consumer.bus];
    [listenerB2 expectInvocations:0];
    [obsB registerObserverListener:listenerB2 triggerOnExisting:NO];

    verifyObjects allListeners2 = ^BOOL() {
        return ((0 == listenerA.counter) &&
                (0 == listenerB.counter) &&
                (0 == listenerB2.counter) &&
                (0 == listenerAB.counter));
    };

    [listenerA expectInvocations:0];
    [listenerB expectInvocations:1];
    [listenerB2 expectInvocations:1];
    [listenerAB expectInvocations:0];
    [provider registerObjectWithName:kObserverTestsObjectB];
    STAssertTrue(YES == [self waitForBlock:allListeners2 msToWait:MAX_WAIT_MS], @"Not detected discovery correctly");

    // Are all objects back where they belong?
    STAssertTrue(2 == [self countProxies:obsA], @"Observer A has incorrect number of proxies");
    STAssertTrue(2 == [self countProxies:obsB], @"Observer B has incorrect number of proxies");
    STAssertTrue(1 == [self countProxies:obsAB], @"Observer AB has incorrect number of proxies");

    // 6. Test multiple observers for the same set of interfaces
    AJNObserver *obsB2 = [[AJNObserver alloc]initWithProxyType:[TestJustBProxy class]
                                                 busAttachment:consumer.bus
                                           mandatoryInterfaces:[NSArray arrayWithObject:kObserverTestsInterfaceNameB]];
    [obsB unregisterObserverListener:listenerB2];
    [listenerB2 reset];

    [listenerA expectInvocations:0];
    [listenerB expectInvocations:0];
    [listenerB2 expectInvocations:2];
    [listenerAB expectInvocations:0];
    [obsB2 registerObserverListener:listenerB2 triggerOnExisting:YES];
    STAssertTrue(YES == [self waitForBlock:allListeners2 msToWait:MAX_WAIT_MS], @"Not detected discovery correctly");
    [obsA unregisterAllObserverListeners];
    [obsB unregisterAllObserverListeners];
    [obsB2 unregisterAllObserverListeners];
    [obsAB unregisterAllObserverListeners];

    obsA = nil;
    obsB = nil;
    obsB2= nil;
    obsAB= nil;
    listenerA = nil;
    listenerB = nil;
    listenerB2= nil;
    listenerAB= nil;
    proxy = nil;
}

- (void)testSimpleDiscovery
{
    Participant *provider = [[Participant alloc]initWithInterfaceRegistration:NO];
    Participant *consumer = [[Participant alloc]initWithInterfaceRegistration:YES];
    [self simpleScenario:provider consumer:consumer];
    provider = nil;
    consumer = nil;
}

- (void)testSimpleSelfDiscovery
{
    Participant *provcons = [[Participant alloc]initWithInterfaceRegistration:YES];
    [self simpleScenario:provcons consumer:provcons];
    provcons = nil;
}

- (void)testRejection
{
    Participant *willing = [[Participant alloc]initWithInterfaceRegistration:NO];
    Participant *doubtful = [[Participant alloc]initWithInterfaceRegistration:NO];
    Participant *unwilling = [[Participant alloc]initWithInterfaceRegistration:NO];
    Participant *consumer = [[Participant alloc]initWithInterfaceRegistration:YES];

    [willing createObjectWithName:kObserverTestsObjectA];
    [doubtful createObjectWithName:kObserverTestsObjectBoth];
    [unwilling createObjectWithName:kObserverTestsObjectBoth];

    unwilling.acceptSessions = NO;

    ObserverListener *listener = [[ObserverListener alloc]initWithBusAttachment:consumer.bus];
    AJNObserver *obs = [[AJNObserver alloc]initWithProxyType:[TestJustAProxy class]
                                               busAttachment:consumer.bus
                                         mandatoryInterfaces:[NSArray arrayWithObject:kObserverTestsInterfaceNameA]];
    [obs registerObserverListener:listener triggerOnExisting:YES];

    [listener expectInvocations:2];
    [willing registerObjectWithName:kObserverTestsObjectA];
    [doubtful registerObjectWithName:kObserverTestsObjectBoth];
    [unwilling registerObjectWithName:kObserverTestsObjectBoth];

    // Get the counter object of the listener
    verifyObjects verifylistener = ^BOOL() {
        return (0 == listener.counter);
    };

    STAssertTrue(YES == [self waitForBlock:verifylistener msToWait:MAX_WAIT_MS], @"Objects were not discovered correctly");

    // Now let doubtful kill the connection
    // First make sure doubtful has a connection to begin with.
    verifyObjects verifySessions = ^BOOL() {
        return (YES == [doubtful hasSessionWith:consumer]);
    };
    STAssertTrue(YES == [self waitForBlock:verifySessions msToWait:MAX_WAIT_MS], @"Session not closed properly");

    [listener expectInvocations:1];
    [doubtful closeSessionForParticipant:consumer];
    STAssertTrue(YES == [self waitForBlock:verifylistener msToWait:MAX_WAIT_MS], @"Objects were not discovered correctly");

    verifyObjects verifyProxies = ^BOOL() {
        return (1 == [self countProxies:obs]);
    };
    // there should only be one object left
    STAssertTrue(YES == [self waitForBlock:verifyProxies msToWait:MAX_WAIT_MS], @"Number of remaining proxies is not  correct");

    // unannounce and reannounce, connection should be restored
    [listener expectInvocations:1];
    [doubtful unregisterObjectWithName:kObserverTestsObjectBoth];
    [doubtful registerObjectWithName:kObserverTestsObjectBoth];
    STAssertTrue(YES == [self waitForBlock:verifylistener msToWait:MAX_WAIT_MS], @"Objects were not discovered correctly");

    // now there should only be two objects
    verifyObjects verifyBothProxies = ^BOOL() {
        return (2 == [self countProxies:obs]);
    };
    STAssertTrue(YES == [self waitForBlock:verifyBothProxies msToWait:MAX_WAIT_MS], @"Number of remaining proxies is not  correct");

    [obs unregisterObserverListener:listener];
    obs = nil;
    listener = nil;
    consumer = nil;
    willing = nil;
    doubtful = nil;
    unwilling = nil;
}

- (void)testCreateDelete
{
    Participant *provider = [[Participant alloc]initWithInterfaceRegistration:NO];
    Participant *consumer = [[Participant alloc]initWithInterfaceRegistration:YES];
    [provider createObjectWithName:kObserverTestsObjectA];
    [provider createObjectWithName:kObserverTestsObjectBoth];
    [provider createObjectWithName:kObserverTestsObjectBoth2];

    ObserverListener *listener = [[ObserverListener alloc]initWithBusAttachment:consumer.bus];
    AJNObserver *obs = [[AJNObserver alloc]initWithProxyType:[TestJustAProxy class]
                                               busAttachment:consumer.bus
                                         mandatoryInterfaces:[NSArray arrayWithObject:kObserverTestsInterfaceNameA]];
    [obs registerObserverListener:listener triggerOnExisting:YES];

    // Get the signaling objects of the listeners
    verifyObjects verifylistener = ^BOOL() {
        return (0 == listener.counter);
    };

    [listener expectInvocations:3];
    [provider registerObjectWithName:kObserverTestsObjectA];
    [provider registerObjectWithName:kObserverTestsObjectBoth];
    [provider registerObjectWithName:kObserverTestsObjectBoth2];

    STAssertTrue(YES == [self waitForBlock:verifylistener msToWait:MAX_WAIT_MS], @"Objects were not discovered correctly");

    // Now create and destroy some observers

    ObserverListener *dummy = [[ObserverListener alloc]initWithBusAttachment:consumer.bus];
    AJNObserver *spark = [[AJNObserver alloc]initWithProxyType:[TestJustAProxy class]
                                               busAttachment:consumer.bus
                                         mandatoryInterfaces:[NSArray arrayWithObject:kObserverTestsInterfaceNameA]];
    spark = nil;
    AJNObserver *flame = [[AJNObserver alloc]initWithProxyType:[TestJustAProxy class]
                                                 busAttachment:consumer.bus
                                           mandatoryInterfaces:[NSArray arrayWithObject:kObserverTestsInterfaceNameA]];
    [flame registerObserverListener:dummy triggerOnExisting:YES];
    flame = nil;

    spark = [[AJNObserver alloc]initWithProxyType:[TestJustAProxy class]
                                    busAttachment:consumer.bus
                              mandatoryInterfaces:[NSArray arrayWithObject:kObserverTestsInterfaceNameA]];
    flame = [[AJNObserver alloc]initWithProxyType:[TestJustAProxy class]
                                    busAttachment:consumer.bus
                              mandatoryInterfaces:[NSArray arrayWithObject:kObserverTestsInterfaceNameA]];
    [flame registerObserverListener:dummy triggerOnExisting:YES];
    flame = nil;
    spark = nil;

    spark = [[AJNObserver alloc]initWithProxyType:[TestJustAProxy class]
                                    busAttachment:consumer.bus
                              mandatoryInterfaces:[NSArray arrayWithObject:kObserverTestsInterfaceNameA]];
    flame = [[AJNObserver alloc]initWithProxyType:[TestJustAProxy class]
                                    busAttachment:consumer.bus
                              mandatoryInterfaces:[NSArray arrayWithObject:kObserverTestsInterfaceNameA]];
    [flame registerObserverListener:dummy triggerOnExisting:YES];
    flame = nil;
    spark = nil;

    // create some movement on the bus to see if there are any lingering
    // traces of spark and flame that create problems
    [listener expectInvocations:3];
    [provider unregisterObjectWithName:kObserverTestsObjectA];
    [provider unregisterObjectWithName:kObserverTestsObjectBoth];
    [provider unregisterObjectWithName:kObserverTestsObjectBoth2];

    STAssertTrue(YES == [self waitForBlock:verifylistener msToWait:MAX_WAIT_MS], @"Objects were not detected as lost correctly");

    [obs unregisterObserverListener:listener];
    obs = nil;
    listener = nil;
    provider = nil;
    consumer = nil;
}

- (void)testListenTwice
{
    Participant *provider = [[Participant alloc]initWithInterfaceRegistration:NO];
    Participant *consumer = [[Participant alloc]initWithInterfaceRegistration:YES];
    [provider createObjectWithName:kObserverTestsObjectA];
    [provider createObjectWithName:kObserverTestsObjectBoth];
    [provider createObjectWithName:kObserverTestsObjectBoth2];

    ObserverListener *listener = [[ObserverListener alloc]initWithBusAttachment:consumer.bus];
    // We're going to attach this listener to two identical observers, so we expect to get discoveredObject twice for each bus object.
    listener.tolerateAlreadyDiscoveredObjects = YES;
    verifyObjects verifylistener = ^BOOL() {
        return (0 == listener.counter);
    };

    AJNObserver *obs = [[AJNObserver alloc]initWithProxyType:[TestJustAProxy class]
                                               busAttachment:consumer.bus
                                         mandatoryInterfaces:[NSArray arrayWithObject:kObserverTestsInterfaceNameA]];
    [obs registerObserverListener:listener triggerOnExisting:YES];

    AJNObserver *obs2 = [[AJNObserver alloc]initWithProxyType:[TestJustAProxy class]
                                                busAttachment:consumer.bus
                                          mandatoryInterfaces:[NSArray arrayWithObject:kObserverTestsInterfaceNameA]];
    [obs2 registerObserverListener:listener triggerOnExisting:YES];

    [listener expectInvocations:6]; // 3 events, times 2 observers feeding the listener with callbacks
    [provider registerObjectWithName:kObserverTestsObjectA];
    [provider registerObjectWithName:kObserverTestsObjectBoth];
    [provider registerObjectWithName:kObserverTestsObjectBoth2];

    STAssertTrue(YES == [self waitForBlock:verifylistener msToWait:MAX_WAIT_MS], @"Objects were not discovered correctly");

    [obs2 unregisterObserverListener:listener];
    obs2 = nil;

    // one observer is gone, so we expect to see every event just once.
    [listener expectInvocations:3];
    [provider unregisterObjectWithName:kObserverTestsObjectA];
    [provider unregisterObjectWithName:kObserverTestsObjectBoth];
    [provider unregisterObjectWithName:kObserverTestsObjectBoth2];

    STAssertTrue(YES == [self waitForBlock:verifylistener msToWait:MAX_WAIT_MS], @"Objects were not lost as expected");

    [obs unregisterObserverListener:listener];
    obs = nil;
    listener = nil;
    provider = nil;
    consumer = nil;
}

- (void)testMulti
{
    // multiple providers, multiple consumers
    Participant *one = [[Participant alloc]initWithInterfaceRegistration:YES];
    [one createObjectWithName:kObserverTestsObjectA];
    [one createObjectWithName:kObserverTestsObjectB];
    [one createObjectWithName:kObserverTestsObjectBoth];

    Participant *two = [[Participant alloc]initWithInterfaceRegistration:YES];
    [two createObjectWithName:kObserverTestsObjectA];
    [two createObjectWithName:kObserverTestsObjectB];
    [two createObjectWithName:kObserverTestsObjectBoth];

    // Observers + listeners
    ObserverListener *listAone = [[ObserverListener alloc]initWithBusAttachment:one.bus];
    AJNObserver *obsAone = [[AJNObserver alloc]initWithProxyType:[TestJustAProxy class]
                                               busAttachment:one.bus
                                         mandatoryInterfaces:[NSArray arrayWithObject:kObserverTestsInterfaceNameA]];
    [obsAone registerObserverListener:listAone triggerOnExisting:YES];
    ObserverListener *listBone = [[ObserverListener alloc]initWithBusAttachment:one.bus];
    AJNObserver *obsBone = [[AJNObserver alloc]initWithProxyType:[TestJustBProxy class]
                                                   busAttachment:one.bus
                                             mandatoryInterfaces:[NSArray arrayWithObject:kObserverTestsInterfaceNameB]];
    [obsBone registerObserverListener:listBone triggerOnExisting:YES];
    ObserverListener *listABone = [[ObserverListener alloc]initWithBusAttachment:one.bus];
    AJNObserver *obsABone = [[AJNObserver alloc]initWithProxyType:[TestBothProxy class]
                                                   busAttachment:one.bus
                                             mandatoryInterfaces:[NSArray arrayWithObjects:kObserverTestsInterfaceNameA, kObserverTestsInterfaceNameB,nil]];
    [obsABone registerObserverListener:listABone triggerOnExisting:YES];

    ObserverListener *listAtwo = [[ObserverListener alloc]initWithBusAttachment:two.bus];
    AJNObserver *obsAtwo = [[AJNObserver alloc]initWithProxyType:[TestJustAProxy class]
                                                   busAttachment:two.bus
                                             mandatoryInterfaces:[NSArray arrayWithObject:kObserverTestsInterfaceNameA]];
    [obsAtwo registerObserverListener:listAtwo triggerOnExisting:YES];
    ObserverListener *listBtwo = [[ObserverListener alloc]initWithBusAttachment:two.bus];
    AJNObserver *obsBtwo = [[AJNObserver alloc]initWithProxyType:[TestJustBProxy class]
                                                   busAttachment:two.bus
                                             mandatoryInterfaces:[NSArray arrayWithObject:kObserverTestsInterfaceNameB]];
    [obsBtwo registerObserverListener:listBtwo triggerOnExisting:YES];
    ObserverListener *listABtwo = [[ObserverListener alloc]initWithBusAttachment:two.bus];
    AJNObserver *obsABtwo = [[AJNObserver alloc]initWithProxyType:[TestBothProxy class]
                                                    busAttachment:two.bus
                                              mandatoryInterfaces:[NSArray arrayWithObjects:kObserverTestsInterfaceNameA, kObserverTestsInterfaceNameB,nil]];
    [obsABtwo registerObserverListener:listABtwo triggerOnExisting:YES];

    [listAone expectInvocations:4];
    [listBone expectInvocations:4];
    [listABone expectInvocations:2];
    [listAtwo expectInvocations:4];
    [listBtwo expectInvocations:4];
    [listABtwo expectInvocations:2];

    [one registerObjectWithName:kObserverTestsObjectA];
    [one registerObjectWithName:kObserverTestsObjectB];
    [one registerObjectWithName:kObserverTestsObjectBoth];
    [two registerObjectWithName:kObserverTestsObjectA];
    [two registerObjectWithName:kObserverTestsObjectB];
    [two registerObjectWithName:kObserverTestsObjectBoth];

    // Get the signaling objects of the listeners
    verifyObjects verifyAlllisteners = ^BOOL() {
        return ((0 == listAone.counter)  &&
                (0 == listBone.counter)  &&
                (0 == listABone.counter) &&
                (0 == listAtwo.counter)  &&
                (0 == listBtwo.counter)  &&
                (0 == listABtwo.counter));
    };

    STAssertTrue(YES == [self waitForBlock:verifyAlllisteners msToWait:MAX_WAIT_MS], @"Objects were not detected correctly");

    verifyObjects verifyDiscoveredProxies = ^BOOL() {
        return ((4 == [self countProxies:obsAone])  &&
                (4 == [self countProxies:obsBone])  &&
                (2 == [self countProxies:obsABone]) &&
                (4 == [self countProxies:obsAtwo])  &&
                (4 == [self countProxies:obsBtwo])  &&
                (2 == [self countProxies:obsABtwo]));
    };
    STAssertTrue(YES == [self waitForBlock:verifyDiscoveredProxies msToWait:MAX_WAIT_MS], @"Number of proxies is not  correct");

    // Now drop all objects
    [listAone expectInvocations:4];
    [listBone expectInvocations:4];
    [listABone expectInvocations:2];
    [listAtwo expectInvocations:4];
    [listBtwo expectInvocations:4];
    [listABtwo expectInvocations:2];

    [one unregisterObjectWithName:kObserverTestsObjectA];
    [one unregisterObjectWithName:kObserverTestsObjectB];
    [one unregisterObjectWithName:kObserverTestsObjectBoth];
    [two unregisterObjectWithName:kObserverTestsObjectA];
    [two unregisterObjectWithName:kObserverTestsObjectB];
    [two unregisterObjectWithName:kObserverTestsObjectBoth];

    STAssertTrue(YES == [self waitForBlock:verifyAlllisteners msToWait:MAX_WAIT_MS], @"Objects were not detected as lost correctly");

    verifyObjects verifyLostProxies = ^BOOL() {
        return ((0 == [self countProxies:obsAone])  &&
                (0 == [self countProxies:obsBone])  &&
                (0 == [self countProxies:obsABone]) &&
                (0 == [self countProxies:obsAtwo])  &&
                (0 == [self countProxies:obsBtwo])  &&
                (0 == [self countProxies:obsABtwo]));
    };
    STAssertTrue(YES == [self waitForBlock:verifyLostProxies msToWait:MAX_WAIT_MS], @"Number of proxies is not  correct");

    [obsAone unregisterObserverListener:listAone];
    [obsBone unregisterObserverListener:listBone];
    [obsABone unregisterObserverListener:listABone];
    [obsAtwo unregisterObserverListener:listAtwo];
    [obsBtwo unregisterObserverListener:listBtwo];
    [obsABtwo unregisterObserverListener:listABtwo];
    obsAone = nil;
    obsBone = nil;
    obsABone = nil;
    obsAtwo = nil;
    obsBtwo = nil;
    obsABtwo = nil;
    listAone = nil;
    listBone = nil;
    listABone = nil;
    listAtwo = nil;
    listBtwo = nil;
    listABtwo = nil;
    one = nil;
    two = nil;
}

- (void)testObserverSanity
{
    // Test basic construction with NULLs of the Observer.
    // If the number of interfaces is not matching the actual number of interfaces in the array,
    // then it's inevitable not to segfault.
    Participant *one = [[Participant alloc]initWithInterfaceRegistration:YES];
    NSArray *mandIntf = [NSArray arrayWithObjects:[NSNull null], nil];
    NSArray *mandIntf2 = [NSArray arrayWithObjects:[NSNull null], [NSNull null], [NSNull null], [NSNull null], [NSNull null], [NSNull null], [NSNull null], [NSNull null], [NSNull null], [NSNull null],nil];

    STAssertTrue(nil == [[AJNObserver alloc]initWithProxyType:[TestJustAProxy class]
                                                busAttachment:one.bus
                                          mandatoryInterfaces:mandIntf], @"Schould not be able to create observer for invalid interface");
    STAssertTrue(nil == [[AJNObserver alloc]initWithProxyType:[TestJustAProxy class]
                                                busAttachment:one.bus
                                          mandatoryInterfaces:mandIntf2], @"Schould not be able to create observer with multiple invalid interfaces");
    STAssertTrue(nil == [[AJNObserver alloc]initWithProxyType:[TestJustAProxy class]
                                                busAttachment:one.bus
                                          mandatoryInterfaces:nil], @"Schould not be able to create observer with missing interfaces");

    // Test using same interface name twice
    NSArray *doubleIntfA = [NSArray arrayWithObjects:kObserverTestsInterfaceNameA,kObserverTestsInterfaceNameA, nil];
    AJNObserver * obsA = [[AJNObserver alloc]initWithProxyType:[TestJustAProxy class]
                                                 busAttachment:one.bus
                                           mandatoryInterfaces:doubleIntfA];
    STAssertTrue(nil != obsA, @"Schould be able to create observer with twice the same interface");

    [one createObjectWithName:kObserverTestsObjectA];
    ObserverListener *listenerA = [[ObserverListener alloc]initWithBusAttachment:one.bus];
    [obsA registerObserverListener:listenerA triggerOnExisting:YES];

    // Get the object counts of the listeners
    verifyObjects checkListener = ^BOOL() {
        return (0 == listenerA.counter);
    };

    // Should be triggered only once on object registration although we have duplicate interfaces
    [listenerA expectInvocations:1];
    [one registerObjectWithName:kObserverTestsObjectA];

    STAssertTrue(YES == [self waitForBlock:checkListener msToWait:MAX_WAIT_MS], @"Object was not discovered correctly");

    // Make sure we have only one proxy for the remote object implementing duplicate interfaces
    verifyObjects verifyProxies = ^BOOL() {
        return (1 == [self countProxies:obsA]);
    };
    STAssertTrue(YES == [self waitForBlock:verifyProxies msToWait:MAX_WAIT_MS], @"Number of remaining proxies is not  correct");

    // Should be triggered only once on object un-registration although we have duplicate interfaces
    [listenerA expectInvocations:1];
    [one unregisterObjectWithName:kObserverTestsObjectA];
    STAssertTrue(YES == [self waitForBlock:checkListener msToWait:MAX_WAIT_MS], @"Object was not discovered as lost correctly");
    [obsA unregisterObserverListener:listenerA];

    obsA = nil;
    listenerA = nil;
    one = nil;
}

- (void)testRegisterListenerTwice
{
    // Reuse the same listener for the same observer
    Participant *provider = [[Participant alloc]initWithInterfaceRegistration:NO];
    Participant *consumer = [[Participant alloc]initWithInterfaceRegistration:YES];
    [provider createObjectWithName:kObserverTestsObjectA];

    ObserverListener *listener = [[ObserverListener alloc]initWithBusAttachment:consumer.bus];
    listener.tolerateAlreadyDiscoveredObjects = YES;
    AJNObserver *obs = [[AJNObserver alloc]initWithProxyType:[TestJustAProxy class]
                                               busAttachment:consumer.bus
                                         mandatoryInterfaces:[NSArray arrayWithObject:kObserverTestsInterfaceNameA]];
    [obs registerObserverListener:listener triggerOnExisting:YES];
    [obs registerObserverListener:listener triggerOnExisting:YES]; // Intentional

    // Should be triggered twice on object registration as we registered the listener twice
    [listener expectInvocations:2];
    [provider registerObjectWithName:kObserverTestsObjectA];

    // Get the counter object of the listener
    verifyObjects verifylistener = ^BOOL() {
        return (0 == listener.counter);
    };

    STAssertTrue(YES == [self waitForBlock:verifylistener msToWait:MAX_WAIT_MS], @"Objects were not discovered correctly");

    // Should be triggered twice on object un-registration as we registered the listener twice
    [listener expectInvocations:2];
    [provider unregisterObjectWithName:kObserverTestsObjectA];
    STAssertTrue(YES == [self waitForBlock:verifylistener msToWait:MAX_WAIT_MS], @"Objects were not discovered as lost correctly");

    [obs unregisterObserverListener:listener];

    // Should be triggered once on object registration as we removed one listener
    [listener expectInvocations:1];
    [provider registerObjectWithName:kObserverTestsObjectA];

    STAssertTrue(YES == [self waitForBlock:verifylistener msToWait:MAX_WAIT_MS], @"Object was not re-discovered correctly");

    [obs unregisterObserverListener:listener];
    obs = nil;
    provider = nil;
    listener = nil;
    consumer = nil;
}

- (void)testAnnounceLogicSanity
{
    Participant *provider = [[Participant alloc]initWithInterfaceRegistration:NO];
    Participant *consumer = [[Participant alloc]initWithInterfaceRegistration:YES];
    ObserverListener *listenerA = [[ObserverListener alloc]initWithBusAttachment:consumer.bus];
    ObserverListener *listenerB = [[ObserverListener alloc]initWithBusAttachment:consumer.bus];

    [provider createObjectWithName:kObserverTestsObjectA];
    [provider createObjectWithName:kObserverTestsObjectB];

    [provider registerObjectWithName:kObserverTestsObjectA];
    [provider registerObjectWithName:kObserverTestsObjectB];

    {
        AJNObserver *obsA = [[AJNObserver alloc]initWithProxyType:[TestJustAProxy class]
                                                   busAttachment:consumer.bus
                                             mandatoryInterfaces:[NSArray arrayWithObject:kObserverTestsInterfaceNameA]];
        // Object with intfA was at least discovered
        [listenerA expectInvocations:1];
        [obsA registerObserverListener:listenerA triggerOnExisting:YES];

        // Get the counter object of the listener
        verifyObjects verifylistenerA = ^BOOL() {
            return (0 == listenerA.counter);
        };
        STAssertTrue(YES == [self waitForBlock:verifylistenerA msToWait:MAX_WAIT_MS], @"Objects were not discovered correctly");
        [obsA unregisterObserverListener:listenerA];
        obsA = nil;

        AJNObserver *obsB = [[AJNObserver alloc]initWithProxyType:[TestJustBProxy class]
                                                   busAttachment:consumer.bus
                                             mandatoryInterfaces:[NSArray arrayWithObject:kObserverTestsInterfaceNameB]];

        // Object with intfB was at least discovered
        [listenerB expectInvocations:1];
        [obsB registerObserverListener:listenerB triggerOnExisting:YES];

        // Get the counter object of the listener
        verifyObjects verifylistenerB = ^BOOL() {
            return (0 == listenerB.counter);
        };
        STAssertTrue(YES == [self waitForBlock:verifylistenerB msToWait:MAX_WAIT_MS], @"Objects were not discovered correctly");
        [obsB unregisterObserverListener:listenerB];
        obsB = nil;
    }

    // Try creating Observer on IntfB after destroying Observer on InftA
    {
        AJNObserver *obsA = [[AJNObserver alloc]initWithProxyType:[TestJustAProxy class]
                                                    busAttachment:consumer.bus
                                              mandatoryInterfaces:[NSArray arrayWithObject:kObserverTestsInterfaceNameA]];
        // Object with intfA was at least discovered
        [listenerA reset];
        [listenerA expectInvocations:1];
        [obsA registerObserverListener:listenerA triggerOnExisting:YES];

        // Get the counter object of the listener
        verifyObjects verifylistenerA = ^BOOL() {
            return (0 == listenerA.counter);
        };

        STAssertTrue(YES == [self waitForBlock:verifylistenerA msToWait:MAX_WAIT_MS], @"Objects were not discovered correctly");
        [obsA unregisterObserverListener:listenerA];
        obsA = nil;
    }

    AJNObserver *obsB = [[AJNObserver alloc]initWithProxyType:[TestJustBProxy class]
                                                busAttachment:consumer.bus
                                          mandatoryInterfaces:[NSArray arrayWithObject:kObserverTestsInterfaceNameB]];

    // Object with intfB was at least discovered
    [listenerB reset];
    [listenerB expectInvocations:1];
    [obsB registerObserverListener:listenerB triggerOnExisting:YES];

    // Get the counter object of the listener
    verifyObjects verifylistenerB = ^BOOL() {
        return (0 == listenerB.counter);
    };

    STAssertTrue(YES == [self waitForBlock:verifylistenerB msToWait:MAX_WAIT_MS], @"Objects were not discovered correctly");
    [obsB unregisterObserverListener:listenerB];

    [provider unregisterObjectWithName:kObserverTestsObjectA];
    [provider unregisterObjectWithName:kObserverTestsObjectB];

    obsB = nil;
    provider = nil;
    listenerA = nil;
    listenerB = nil;
    consumer = nil;
}


@end
