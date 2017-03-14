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

#import "DoorObserverAllJoynService.h"
#import "AJNAboutObject.h"
#import "AJNAboutData.h"
#import "DoorObject.h"
#import "AJNInit.h"
#import "AJNObserverListener.h"
#import "AJNObserver.h"

static NSString * const kDoorPathPrefix = @"/org/alljoyn/sample/door/";
static const NSInteger kDoorServicePort = 42;
static NSString *const lettesForRandomString = @"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static NSString * const kDoorInterfaceName = @"com.example.Door";

@interface DoorObserverAllJoynService () <AJNSessionListener, AJNSessionPortListener, AJNObserverListener>

@property (nonatomic, strong) AJNBusAttachment *bus;
@property (nonatomic, strong) AJNAboutObject *aboutObject;
@property (nonatomic, strong) AJNAboutData *aboutData;
@property (nonatomic, strong) AJNObserver *doorObserver;
@property (nonatomic, strong) NSMutableArray<DoorObject *> *localDoorObjects;
@property (nonatomic, strong, readonly) NSMutableArray<DoorObjectProxy *>* proxyDoorsObjects;
@property (nonatomic, weak) UITableView *uiTableView;

- (BOOL)generateAboutData;
- (void)createDoorObserver;
- (NSString *)randomStringWithLength:(int) len;

@end

@implementation DoorObserverAllJoynService

- (id)init
{
    if (self = [super init]) {
        _isServiceStarted = false;
        _proxyDoorsObjects = [[NSMutableArray alloc] init];
        _localDoorObjects = [[NSMutableArray alloc] init];
        return self;
    }
    return nil;
};

- (BOOL)startService
{
    QStatus status = [AJNInit alljoynInit];
    if (status == ER_OK) {
        status = [AJNInit alljoynRouterInit];
        if (status != ER_OK) {
            NSLog(@"alljoynRouterInit fails with error: %u", status);
            [AJNInit alljoynShutdown];
        }
    } else {
        NSLog(@"alljoynInit fails with error: %u", status);
        return NO;
    }
    
    _bus = [[AJNBusAttachment alloc] initWithApplicationName:@"DoorObserver" allowRemoteMessages:YES];

    status = [_bus start];
    if (status != ER_OK) {
        NSLog(@"Alljoyn bus start fails with error: %u", status);
        _bus = nil;
        return NO;
    }

    status = [_bus connectWithArguments:@"null:"];
    if (status != ER_OK) {
        NSLog(@"Alljoyn bus connect fails with error: %u", status);
        [_bus stop];
        _bus = nil;
        return NO;
    }

    _aboutObject = [[AJNAboutObject alloc] initWithBusAttachment:_bus withAnnounceFlag:UNANNOUNCED];

    AJNSessionOptions *sessionOptions =
    [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages
                                supportsMultipoint:YES
                                         proximity:kAJNProximityAny
                                     transportMask:kAJNTransportMaskAny];

    status = [_bus bindSessionOnPort:kDoorServicePort
                         withOptions:sessionOptions
                        withDelegate:self];
    if (status != ER_OK) {
        NSLog(@"Alljoyn binding session fails with error: %u", status);
        [self stopService];
        return NO;
    }

    BOOL isGeneratedAboutDataIsValid = [self generateAboutData];
    if (!isGeneratedAboutDataIsValid) {
        NSLog(@"Alljoyn generate about data fails with error: %u", status);
        [self stopService];
        return NO;
    }

    [self createDoorObserver];

    _isServiceStarted = true;

    return YES;
};

- (void)stopService
{
    [_bus disconnect];
    [_bus stop];
    [_bus waitUntilStopCompleted];
    _bus = nil;
    _isServiceStarted = false;
};

- (void)setTableView:(UITableView *) uiTableView
{
    _uiTableView = uiTableView; //TODO: create listener for notification
}

- (void)clearTableView
{
    _uiTableView = nil;
}

- (QStatus)produceDoorAtLocation:(NSString *)location
{
    NSString *path = [NSString stringWithFormat:@"%@%@",kDoorPathPrefix,[self randomStringWithLength:8]];

    DoorObject* door = nil;
    door = [[DoorObject alloc] initWithLocation:location keyCode:@1111 isOpen:NO busAttachment:_bus path:path];

    [_bus registerBusObject:door];
    [_localDoorObjects addObject:door];

    QStatus status = [self.aboutObject announceForSessionPort:kDoorServicePort withAboutDataListener:self.aboutData];
    NSLog(@"Trying to produce door at location %@. Status = %u", location, status);

    return status;
};

- (BOOL)clickOnProxyDoorAtIndex:(NSInteger)index
{
    DoorObjectProxy *tappedItem = [_proxyDoorsObjects objectAtIndex:index];
    if (tappedItem.IsOpen) {
        [tappedItem close];
        return NO;
    } else {
        [tappedItem open];
        return YES;
    }
}

- (BOOL)generateAboutData
{
    // Setup the about data
    // The default language is specified in the constructor. If the default language
    // is not specified any Field that should be localized will return an error
    _aboutData = [[AJNAboutData alloc] initWithLanguage:@"en"];
    //AppId is a 128bit uuid
    uint8_t appId[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    [_aboutData setAppId:appId];
    [_aboutData setDeviceName:@"Device Name" andLanguage:@"en"];
    //DeviceId is a string encoded 128bit UUID
    [_aboutData setDeviceId:@"avec-awe1213-1234559xvc123"];
    [_aboutData setAppName:@"App Name" andLanguage:@"en"];
    [_aboutData setManufacturer:@"Manufacturer" andLanguage:@"en"];
    [_aboutData setModelNumber:@"ModelNo"];
    [_aboutData setDescription:@"A poetic description of this application" andLanguage:@"en"];
    [_aboutData setDateOfManufacture:@"DateOfManufacture"];
    [_aboutData setSoftwareVersion:@"1.0"];
    [_aboutData setHardwareVersion:@"16.10.00"];
    [_aboutData setSupportUrl:@"some.random.url"];

    return [_aboutData isValid];
}

-(void)createDoorObserver
{
    // register door interface
    QStatus status = ER_OK;
    AJNInterfaceDescription *interfaceDescription = [self.bus createInterfaceWithName:@"com.example.Door" withInterfaceSecPolicy:AJN_IFC_SECURITY_OFF];
    status = [interfaceDescription addPropertyWithName:@"IsOpen" signature:@"b" accessPermissions:kAJNInterfacePropertyAccessReadWriteFlag];
    if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
        @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add property to interface:  IsOpen" userInfo:nil];
    }
    status = [interfaceDescription addPropertyWithName:@"Location" signature:@"s" accessPermissions:kAJNInterfacePropertyAccessReadWriteFlag];
    if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
        @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add property to interface:  Location" userInfo:nil];
    }
    status = [interfaceDescription addPropertyWithName:@"KeyCode" signature:@"u" accessPermissions:kAJNInterfacePropertyAccessReadWriteFlag];
    if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
        @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add property to interface:  KeyCode" userInfo:nil];
    }

    status = [interfaceDescription addMethodWithName:@"Open" inputSignature:@"" outputSignature:@"" argumentNames:[NSArray arrayWithObjects: nil]];
    if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
        @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add method to interface: Open" userInfo:nil];
    }
    status = [interfaceDescription addMethodWithName:@"Close" inputSignature:@"" outputSignature:@"" argumentNames:[NSArray arrayWithObjects: nil]];
    if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
        @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add method to interface: Close" userInfo:nil];
    }
    status = [interfaceDescription addMethodWithName:@"KnockAndRun" inputSignature:@"" outputSignature:@"" argumentNames:[NSArray arrayWithObjects: nil]];
    if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
        @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add method to interface: KnockAndRun" userInfo:nil];
    }

    status = [interfaceDescription addSignalWithName:@"PersonPassedThrough" inputSignature:@"s" argumentNames:[NSArray arrayWithObjects:@"name", nil]];
    if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
        @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add signal to interface:  PersonPassedThrough" userInfo:nil];
    }
    [interfaceDescription activate];

    _doorObserver = [[AJNObserver alloc]initWithProxyType:[DoorObjectProxy class]
                                                busAttachment:self.bus
                                          mandatoryInterfaces:[NSArray arrayWithObject:kDoorInterfaceName]];

    [_doorObserver registerObserverListener:self triggerOnExisting:YES];
}

#pragma mark - AJNObserverListener protocol

-(void)didDiscoverObject:(AJNProxyBusObject *)obj forObserver:(AJNObserver *)observer
{
    [_proxyDoorsObjects addObject:(DoorObjectProxy*)obj];
    dispatch_async(dispatch_get_main_queue(), ^{
        [_uiTableView reloadData];
    });
}

-(void)didLoseObject:(AJNProxyBusObject *)obj forObserver:(AJNObserver *)observer;
{
    // AJNProxyBusObject does not support hash and isEqual
    NSUInteger idx = 0;
    for (;idx<[_localDoorObjects count];++idx){
        NSString *objBusName = [NSString stringWithString:((AJNBusAttachment *)[obj valueForKey:@"bus"]).uniqueName];
        NSString *itemBusName = [NSString stringWithString:((AJNBusAttachment *)[[_localDoorObjects objectAtIndex:idx] valueForKey:@"bus"]).uniqueName];
        if ([obj.path isEqualToString:((AJNProxyBusObject *)[_localDoorObjects objectAtIndex:idx]).path] &&
            [objBusName isEqualToString:itemBusName]){
            [_localDoorObjects removeObjectAtIndex:idx];
            break;
        }
    }
}


-(NSString *)randomStringWithLength: (int) len
{
    NSMutableString *randomString = [NSMutableString stringWithCapacity: len];
    for (int i=0; i<len; i++) {
        [randomString appendFormat: @"%C", [lettesForRandomString characterAtIndex: arc4random_uniform([lettesForRandomString length])]];
    }
    return randomString;
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
    [_bus setHostedSessionListener:self toSession:sessionId];
}

#pragma mark - UITableViewDataSource protocol

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    // Return the number of rows in the section.
    return [_proxyDoorsObjects count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"DoorPrototypeCell" forIndexPath:indexPath];
    DoorObjectProxy *proxy = [_proxyDoorsObjects objectAtIndex:indexPath.row];
    cell.textLabel.text = proxy.Location;
    if (proxy.IsOpen) {
        cell.accessoryType = UITableViewCellAccessoryCheckmark;
    } else {
        cell.accessoryType = UITableViewCellAccessoryNone;
    }
    return cell;
}

@end
