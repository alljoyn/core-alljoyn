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
#import "AJNClientController.h"
#import "SecureObject.h"
#import "Constants.h"

@interface ViewController () <AJNClientDelegate, AJNAuthenticationListener>

@property (nonatomic, strong) SecureObjectProxy *secureObjectProxy;
@property (nonatomic, strong) AJNClientController *clientController;

- (AJNProxyBusObject *)proxyObjectOnBus:(AJNBusAttachment *)bus inSession:(AJNSessionId)sessionId;
- (void)shouldUnloadProxyObjectOnBus:(AJNBusAttachment *)bus;

- (void)didStartBus:(AJNBusAttachment *)bus;

@end

@implementation ViewController

@synthesize secureObjectProxy = _secureObjectProxy;
@synthesize startButton = _startButton;
@synthesize clientController = _clientController;
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

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view, typically from a nib.
    
    // allocate the client bus controller and set bus properties
    //
    self.clientController = [[AJNClientController alloc] init];
    self.clientController.trafficType = kAJNTrafficMessages;
    self.clientController.proximityOptions = kAJNProximityAny;
    self.clientController.transportMask = kAJNTransportMaskAny;
    self.clientController.allowRemoteMessages = YES;
    self.clientController.multiPointSessionsEnabled = YES;
    self.clientController.delegate = self;
    
}

- (IBAction)didTouchStartButton:(id)sender
{
    if (self.clientController.bus.isStarted) {
        [self.clientController stop];
        [self.startButton setTitle:@"Start" forState:UIControlStateNormal];
    }
    else {
        [self.clientController start];
        [self.startButton setTitle:@"Stop" forState:UIControlStateNormal];        
    }
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)shouldUnloadProxyObjectOnBus:(AJNBusAttachment *)bus
{
    self.secureObjectProxy = nil;
    [self.startButton setTitle:@"Start" forState:UIControlStateNormal];        
}

- (AJNProxyBusObject *)proxyObjectOnBus:(AJNBusAttachment *)bus inSession:(AJNSessionId)sessionId
{
    self.secureObjectProxy = [[SecureObjectProxy alloc] initWithBusAttachment:bus serviceName:self.serviceName objectPath:kServicePath sessionId:sessionId];
    
    // now we need to authenticate
    //
    [self.secureObjectProxy secureConnection:YES];
    
    return self.secureObjectProxy;
}

- (void)didJoinInSession:(AJNSessionId)sessionId withService:(NSString *)serviceName
{
    NSString *pingString = @"Hello AllJoyn!";
    NSString *returnPingString;
    [self didReceiveStatusMessage:[NSString stringWithFormat:@"Sending ping string [%@]", pingString]];
    returnPingString = [self.secureObjectProxy sendPingString:pingString];
    [self didReceiveStatusMessage:[NSString stringWithFormat:@"Ping string returned [%@] from service and %@", returnPingString, returnPingString ? @"was successful" : @"failed"]];
    
    [self.clientController stop];
}

- (void)didStartBus:(AJNBusAttachment *)bus
{
    // enable security for the bus
    //
    NSString *keystoreFilePath = @"Documents/alljoyn_keystore/s_central.ks";
    // Enable the authentication you want to use (ALLJOYN_SRP_KEYX, ALLJOYN_ECDHE_NULL, ALLJOYN_ECDHE_PSK, ALLJOYN_ECDHE_ECDSA)
    QStatus status = [bus enablePeerSecurity:@"ALLJOYN_ECDHE_PSK" authenticationListener:self keystoreFileName:keystoreFilePath sharing:YES];
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
                    "CkzgQdvZSOQMmqOnddsw0BRneCNZhioNMyUoJwec9rMAAAAA"
                    "-----END PRIVATE KEY-----"};
                credentials.privateKey = [NSString stringWithUTF8String:privakteKey];
            }
            if (mask & kAJNSecurityCredentialTypeCertificateChain) {
                const char certchain[] = {
                    "-----BEGIN CERTIFICATE-----\n"
                    "AAAAAZ1LKGlnpVVtV4Sa1TULsxGJR9C53Uq5AH3fxqxJjNdYAAAAAAobbdvBKaw9\n"
                    "eHox7o9fNbN5usuZw8XkSPSmipikYCPJAAAAAAAAAABiToQ8L3KZLwSCetlNJwfd\n"
                    "bbxbo2x/uooeYwmvXbH2uwAAAABFQGcdlcsvhdRxgI4SVziI4hbg2d2xAMI47qVB\n"
                    "ZZsqJAAAAAAAAAAAAAAAAAABYGEAAAAAAAFhjQABMa7uTLSqjDggO0t6TAgsxKNt\n"
                    "+Zhu/jc3s242BE0drNFJAiGa/u6AX5qdR+7RFxVuqm251vKPgWjfwN2AesHrAAAA\n"
                    "ANsNwJl8Z1v5jbqo077qdQIT6aM1jc+pKXdgNMk6loqFAAAAAA==\n"
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
