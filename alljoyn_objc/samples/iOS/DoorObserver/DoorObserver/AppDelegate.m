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

#import "AppDelegate.h"

#import "AJNAboutObject.h"
#import "AJNInit.h"
#import "DoorObject.h"

static NSString * const kDoorPathPrefix = @"/org/alljoyn/sample/door/";
const NSInteger kDoorServicePort = 42;

#pragma mark - Extension
@interface AppDelegate () <AJNSessionListener, AJNSessionPortListener, AJNAboutDataListener>
@property (nonatomic, strong) NSMutableDictionary *aboutData;
@property (nonatomic, strong) AJNAboutObject *aboutObj;
@property (nonatomic, strong) NSMutableArray *localDoorObjects;
@property (nonatomic,readwrite) AJNSessionId sessionId;
@end

#pragma mark -

@implementation AppDelegate

#pragma mark About Info
//--------------------------
// About info
//--------------------------

- (void)initAbout {
    self.aboutData = [[NSMutableDictionary alloc] initWithCapacity:16];

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

    self.aboutObj = [[AJNAboutObject alloc] initWithBusAttachment:self.bus withAnnounceFlag:ANNOUNCED];
}

#pragma mark - AJNAboutDataListener protocol

- (QStatus)getAboutDataForLanguage:(NSString *)language usingDictionary:(NSMutableDictionary **)aboutData
{
    return [self getDefaultAnnounceData:aboutData];
}

-(QStatus)getDefaultAnnounceData:(NSMutableDictionary **)aboutData
{
    *aboutData = self.aboutData;
    return ER_OK;
}

#pragma mark - Local Door
//--------------------------
// Procude a local Door
//--------------------------

NSString *letters = @"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

-(NSString *)randomStringWithLength: (int) len
{
    NSMutableString *randomString = [NSMutableString stringWithCapacity: len];
    for (int i=0; i<len; i++) {
        [randomString appendFormat: @"%C", [letters characterAtIndex: arc4random_uniform([letters length])]];
    }
    return randomString;
}

- (void)produceDoorAtLocation:(NSString *)location
{
    NSString *path = [NSString stringWithFormat:@"%@%@",kDoorPathPrefix,[self randomStringWithLength:8]];

    AJNDoorObject* door = nil;
    door = [[DoorObject alloc] initWithLocation:location keyCode:@1111 isOpen:NO busAttachment:self.bus path:path];

    [self.bus registerBusObject:door];
    [self.aboutObj announceForSessionPort:kDoorServicePort withAboutDataListener:self];

    [self.localDoorObjects addObject:door];
}

#pragma mark - Bus ans Session setup
//--------------------------
// Bus and session set-up
//--------------------------

- (void)startBus {
    self.bus = [[AJNBusAttachment alloc] initWithApplicationName:@"DoorObserver" allowRemoteMessages:YES];
    [self.bus start];
    [self.bus connectWithArguments:@"null:"];

    AJNSessionOptions *sessionOptions =
    [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages
                                supportsMultipoint:YES
                                         proximity:kAJNProximityAny
                                     transportMask:kAJNTransportMaskAny];

    [self.bus bindSessionOnPort:kDoorServicePort
                    withOptions:sessionOptions
                   withDelegate:self];
}

-(void)stopBus {
    [self.bus disconnectWithArguments:@"null:"];
    [self.bus stop];
    [self.bus waitUntilStopCompleted];
    self.bus = nil;
}

#pragma mark - AJNSessionListener protocol

- (void)sessionWasLost:(AJNSessionId)sessionId forReason:(AJNSessionLostReason)reason
{
    NSLog(@"AJNSessionListener::sessionWasLost:%u forReason:%d", sessionId, reason);
}

#pragma mark - AJNSessionPortListener protocol

- (BOOL)shouldAcceptSessionJoinerNamed:(NSString *)joiner onSessionPort:(AJNSessionPort)sessionPort withSessionOptions:(AJNSessionOptions *)options
{
    NSLog(@"AJNSessionPortListener::shouldAcceptSessionJoinerNamed:%@ onSessionPort:%u withSessionOptions:", joiner, sessionPort);
    return TRUE;
}

- (void)didJoin:(NSString *)joiner inSessionWithId:(AJNSessionId)sessionId onSessionPort:(AJNSessionPort)sessionPort
{
    NSLog(@"AJNSessionPortListener::didJoin:%@ inSessionWithId:%u onSessionPort:%u withSessionOptions:", joiner, sessionId, sessionPort);
    [self.bus bindHostedSessionListener:self toSession:sessionId];
}


#pragma mark - UIApplicationDelegate protocol
//--------------------------
// UIApplicationDelegate
//--------------------------

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    // Init Alljoyn
    [AJNInit alljoynInit];
    [AJNInit alljoynRouterInit];

    // Create and start Alljoyn bus
    [self startBus];

    // To support producing local doors
    [self initAbout];
    self.localDoorObjects = [[NSMutableArray alloc] init];

    return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application {
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
    // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
    // Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
}

- (void)applicationWillTerminate:(UIApplication *)application {
    // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
}

#pragma mark - Memory Management

- (void)dealloc
{
    // Cleanup Alljoyn
    [AJNInit alljoynRouterShutdown];
    [AJNInit alljoynShutdown];
}

@end
