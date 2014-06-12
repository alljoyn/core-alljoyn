////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2012, 2014 AllSeen Alliance. All rights reserved.
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

#import "ViewController.h"
#import "AJNServiceController.h"
#import "SecureObject.h"
#import "Constants.h"

@interface ViewController () <AJNServiceDelegate, AJNAuthenticationListener>

@property (nonatomic, strong) SecureObject *secureObject;
@property (nonatomic, strong) AJNServiceController *serviceController;

- (AJNBusObject *)objectOnBus:(AJNBusAttachment *)bus;
- (void)shouldUnloadObjectOnBus:(AJNBusAttachment *)bus;

- (void)didStartBus:(AJNBusAttachment *)bus;

@end

@implementation ViewController

@synthesize secureObject = _secureObject;
@synthesize startButton = _startButton;
@synthesize serviceController = _serviceController;
@synthesize password = _password;

- (NSString *)applicationName
{
    return kAppName;
}

- (NSString *)serviceName
{
    return kServiceName;
}

- (AJNBusNameFlag)serviceNameFlags
{
    return kAJNBusNameFlagDoNotQueue | kAJNBusNameFlagReplaceExisting;
}

- (AJNSessionPort)sessionPort
{
    return kServicePort;
}

- (AJNBusObject *)object
{
    return self.secureObject;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view, typically from a nib.
    
    // allocate the client bus controller and set bus properties
    //
    self.serviceController = [[AJNServiceController alloc] init];
    self.serviceController.trafficType = kAJNTrafficMessages;
    self.serviceController.proximityOptions = kAJNProximityAny;
    self.serviceController.transportMask = kAJNTransportMaskAny;
    self.serviceController.allowRemoteMessages = YES;
    self.serviceController.multiPointSessionsEnabled = YES;
    self.serviceController.delegate = self;
    
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    
    self.passwordLabel.text = self.password;
}

- (IBAction)didTouchStartButton:(id)sender
{
    if (self.serviceController.bus.isStarted) {
        [self.serviceController stop];
        [self.startButton setTitle:@"Start" forState:UIControlStateNormal];
    }
    else {
        [self.serviceController start];
        [self.startButton setTitle:@"Stop" forState:UIControlStateNormal];        
    }
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)shouldUnloadObjectOnBus:(AJNBusAttachment *)bus
{
    self.secureObject = nil;
    [self.startButton setTitle:@"Start" forState:UIControlStateNormal];        
}

- (AJNBusObject *)objectOnBus:(AJNBusAttachment *)bus
{
    self.secureObject = [[SecureObject alloc] initWithBusAttachment:self.serviceController.bus onPath:kServicePath];
    
    return self.secureObject;
}

- (void)didStartBus:(AJNBusAttachment *)bus
{
    // enable security for the bus
    //
    NSString *keystoreFilePath = @"Documents/alljoyn_keystore/s_central.ks";
    // Enable the authentication you want to support
    QStatus status = [bus enablePeerSecurity:@"ALLJOYN_SRP_KEYX ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_ECDSA" authenticationListener:self keystoreFileName:keystoreFilePath sharing:YES];
    NSString *message;
    if (status != ER_OK) {
        message = [NSString stringWithFormat:@"ERROR: Failed to enable security on the bus. %@", [AJNStatus descriptionForStatusCode:status]];
    }
    else {
        message = @"Successfully enabled security for the bus";
    }
    NSLog(@"%@",message);
    [self didReceiveStatusMessage:message];
}

- (void)authenticationUsing:(NSString *)authenticationMechanism forRemotePeer:(NSString *)peerName didCompleteWithStatus:(BOOL)success
{
    NSString *message = [NSString stringWithFormat:@"Authentication %@ %@.", authenticationMechanism, success ? @"successful" : @"failed"];
    NSLog(@"%@", message);
    [self didReceiveStatusMessage:message];
}

- (AJNSecurityCredentials *)requestSecurityCredentialsWithAuthenticationMechanism:(NSString *)authenticationMechanism peerName:(NSString *)peerName authenticationCount:(uint16_t)authenticationCount userName:(NSString *)userName credentialTypeMask:(AJNSecurityCredentialType)mask
{
    NSLog(@"RequestCredentials for authenticating %@ using mechanism %@", peerName, authenticationMechanism);
    AJNSecurityCredentials *credentials = nil;
    if ([authenticationMechanism compare:@"ALLJOYN_SRP_KEYX"] == NSOrderedSame) {
        if (mask & kAJNSecurityCredentialTypePassword) {
            if (authenticationCount <= 3) {
                credentials = [[AJNSecurityCredentials alloc] init];
                credentials.password = self.password;
            }
        }
    }
    else if ([authenticationMechanism compare:@"ALLJOYN_ECDHE_NULL"] == NSOrderedSame) {
        if (authenticationCount <= 3) {
            // Need to send back something other the nil
            credentials = [[AJNSecurityCredentials alloc] init];
        }
    }
    else if ([authenticationMechanism compare:@"ALLJOYN_ECDHE_PSK"] == NSOrderedSame) {
        if (mask & kAJNSecurityCredentialTypePassword) {
            if (authenticationCount <= 3) {
                credentials = [[AJNSecurityCredentials alloc] init];
                credentials.password = self.password;
            }
        }
    }
    else if ([authenticationMechanism compare:@"ALLJOYN_ECDHE_ECDSA"] == NSOrderedSame) {
        if (authenticationCount <= 3) {
            credentials = [[AJNSecurityCredentials alloc] init];
            if (mask & kAJNSecurityCredentialTypePrivateKey) {
                const char privakteKey[] = {
                    "-----BEGIN PRIVATE KEY-----\n"
                    "tV/tGPp7kI0pUohc+opH1LBxzk51pZVM/RVKXHGFjAcAAAAA"
                    "-----END PRIVATE KEY-----"};
                credentials.privateKey = [NSString stringWithUTF8String:privakteKey];
            }
            if (mask & kAJNSecurityCredentialTypeCertificateChain) {
                const char certchain[] = {
                    "-----BEGIN CERTIFICATE-----\n"
                    "AAAAAfUQdhMSDuFWahMG/rFmFbKM06BjIA2Scx9GH+ENLAgtAAAAAIbhHnjAyFys\n"
                    "6DoN2kKlXVCgtHpFiEYszOYXI88QDvC1AAAAAAAAAAC5dRALLg6Qh1J2pVOzhaTP\n"
                    "xI+v/SKMFurIEo2b4S8UZAAAAADICW7LLp1pKlv6Ur9+I2Vipt5dDFnXSBiifTmf\n"
                    "irEWxQAAAAAAAAAAAAAAAAABXLAAAAAAAAFd3AABMa7uTLSqjDggO0t6TAgsxKNt\n"
                    "+Zhu/jc3s242BE0drPcL4K+FOVJf+tlivskovQ3RfzTQ+zLoBH5ZCzG9ua/dAAAA\n"
                    "ACt5bWBzbcaT0mUqwGOVosbMcU7SmhtE7vWNn/ECvpYFAAAAAA==\n"
                    "-----END CERTIFICATE-----"};
                credentials.certificateChain = [NSString stringWithUTF8String:certchain];
            }
        }
    }
    return credentials;
}

- (void)didReceiveStatusMessage:(NSString*)message
{
    dispatch_async(dispatch_get_main_queue(), ^{
        NSMutableString *string = self.eventsTextView.text.length ? [self.eventsTextView.text mutableCopy] : [[NSMutableString alloc] init];
        NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
        [formatter setTimeStyle:NSDateFormatterMediumStyle];
        [formatter setDateStyle:NSDateFormatterShortStyle];
        [string appendFormat:@"[%@]\n",[formatter stringFromDate:[NSDate date]]];
        [string appendString:message];
        [string appendString:@"\n\n"];
        [self.eventsTextView setText:string];
        NSLog(@"%@",message);
        [self.eventsTextView scrollRangeToVisible:NSMakeRange([self.eventsTextView.text length], 0)];
    });
}


@end
