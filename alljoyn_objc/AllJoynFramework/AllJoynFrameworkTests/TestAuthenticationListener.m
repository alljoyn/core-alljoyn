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

#import "TestAuthenticationListener.h"

static NSString *x509Certificate = 
    @"-----BEGIN CERTIFICATE-----\n"
    @"MIIBszCCARwCCQDuCh+BWVBk2DANBgkqhkiG9w0BAQUFADAeMQ0wCwYDVQQKDARN\n"
    @"QnVzMQ0wCwYDVQQDDARHcmVnMB4XDTEwMDUxNzE1MTg1N1oXDTExMDUxNzE1MTg1\n"
    @"N1owHjENMAsGA1UECgwETUJ1czENMAsGA1UEAwwER3JlZzCBnzANBgkqhkiG9w0B\n"
    @"AQEFAAOBjQAwgYkCgYEArSd4r62mdaIRG9xZPDAXfImt8e7GTIyXeM8z49Ie1mrQ\n"
    @"h7roHbn931Znzn20QQwFD6pPC7WxStXJVH0iAoYgzzPsXV8kZdbkLGUMPl2GoZY3\n"
    @"xDSD+DA3m6krcXcN7dpHv9OlN0D9Trc288GYuFEENpikZvQhMKPDUAEkucQ95Z8C\n"
    @"AwEAATANBgkqhkiG9w0BAQUFAAOBgQBkYY6zzf92LRfMtjkKs2am9qvjbqXyDJLS\n"
    @"viKmYe1tGmNBUzucDC5w6qpPCTSe23H2qup27///fhUUuJ/ssUnJ+Y77jM/u1O9q\n"
    @"PIn+u89hRmqY5GKHnUSZZkbLB/yrcFEchHli3vLo4FOhVVHwpnwLtWSpfBF9fWcA\n"
    @"7THIAV79Lg==\n"
    @"-----END CERTIFICATE-----";

static NSString *privateKey = 
    @"-----BEGIN RSA PRIVATE KEY-----\n"
    @"Proc-Type: 4,ENCRYPTED\n"
    @"DEK-Info: AES-128-CBC,0AE4BAB94CEAA7829273DD861B067DBA\n"
    @"\n"
    @"LSJOp+hEzNDDpIrh2UJ+3CauxWRKvmAoGB3r2hZfGJDrCeawJFqH0iSYEX0n0QEX\n"
    @"jfQlV4LHSCoGMiw6uItTof5kHKlbp5aXv4XgQb74nw+2LkftLaTchNs0bW0TiGfQ\n"
    @"XIuDNsmnZ5+CiAVYIKzsPeXPT4ZZSAwHsjM7LFmosStnyg4Ep8vko+Qh9TpCdFX8\n"
    @"w3tH7qRhfHtpo9yOmp4hV9Mlvx8bf99lXSsFJeD99C5GQV2lAMvpfmM8Vqiq9CQN\n"
    @"9OY6VNevKbAgLG4Z43l0SnbXhS+mSzOYLxl8G728C6HYpnn+qICLe9xOIfn2zLjm\n"
    @"YaPlQR4MSjHEouObXj1F4MQUS5irZCKgp4oM3G5Ovzt82pqzIW0ZHKvi1sqz/KjB\n"
    @"wYAjnEGaJnD9B8lRsgM2iLXkqDmndYuQkQB8fhr+zzcFmqKZ1gLRnGQVXNcSPgjU\n"
    @"Y0fmpokQPHH/52u+IgdiKiNYuSYkCfHX1Y3nftHGvWR3OWmw0k7c6+DfDU2fDthv\n"
    @"3MUSm4f2quuiWpf+XJuMB11px1TDkTfY85m1aEb5j4clPGELeV+196OECcMm4qOw\n"
    @"AYxO0J/1siXcA5o6yAqPwPFYcs/14O16FeXu+yG0RPeeZizrdlv49j6yQR3JLa2E\n"
    @"pWiGR6hmnkixzOj43IPJOYXySuFSi7lTMYud4ZH2+KYeK23C2sfQSsKcLZAFATbq\n"
    @"DY0TZHA5lbUiOSUF5kgd12maHAMidq9nIrUpJDzafgK9JrnvZr+dVYM6CiPhiuqJ\n"
    @"bXvt08wtKt68Ymfcx+l64mwzNLS+OFznEeIjLoaHU4c=\n"
    @"-----END RSA PRIVATE KEY-----";


@implementation TestAuthenticationListener

@synthesize bus = _bus;
@synthesize userName = _userName;
@synthesize maximumAuthentications = _maximumAuthentications;
@synthesize keyExpiration = _keyExpiration;

- (id)initOnBus:(AJNBusAttachment*)bus withUserName:(NSString*)userName maximumAuthenticationsAllowed:(NSUInteger)maximumAuthentications
{
    self = [super init];
    if (self) {
        self.bus = bus;
        self.userName = userName;
        self.maximumAuthentications = maximumAuthentications;
    }
    return self;
}

- (AJNSecurityCredentials*)requestSecurityCredentialsWithAuthenticationMechanism:(NSString*)authenticationMechanism peerName:(NSString*)peerName authenticationCount:(uint16_t)authenticationCount userName:(NSString*)userName credentialTypeMask:(AJNSecurityCredentialType)mask
{
    AJNSecurityCredentials *securityCredentials = [[AJNSecurityCredentials alloc] init];
    
    if (authenticationCount > self.maximumAuthentications) {
        return nil;
    }
    
    NSLog(@"RequestCredentials for authenticating %@ using mechanism %@", peerName, authenticationMechanism);
    
    NSString *guid = [self.bus guidForPeerNamed:peerName];
    NSLog(@"Peer guid %@", guid);
    
    if (self.keyExpiration != 0xFFFFFFFF) {
        [securityCredentials setExpirationTime:self.keyExpiration];
    }
    
    if ([authenticationMechanism compare:@"ALLJOYN_SRP_KEYX"] == NSOrderedSame) {
        if (mask & kAJNSecurityCredentialTypePassword) {
            if (authenticationCount == 3) {
                [securityCredentials setPassword:@"123456"];
            } else {
                [securityCredentials setPassword:@"xxxxxx"];
            }
            NSLog(@"AuthListener returning fixed pin \"%@\" for %@\n", securityCredentials.password, authenticationMechanism);
        }
        return securityCredentials;
    }
    
    if ([authenticationMechanism compare:@"ALLJOYN_RSA_KEYX"] == NSOrderedSame) {
        if (mask & kAJNSecurityCredentialTypeCertificateChain) {
            [securityCredentials setCertificateChain:x509Certificate];
        }
        if (mask & kAJNSecurityCredentialTypePrivateKey) {
            [securityCredentials setPrivateKey:privateKey];
        }
        if (mask & kAJNSecurityCredentialTypePassword) {
            [securityCredentials setPassword:@"123456"];
        }
        return securityCredentials;
    }
    
    if ([authenticationMechanism compare:@"ALLJOYN_SRP_LOGON"] == NSOrderedSame) {
        if (mask & kAJNSecurityCredentialTypeUserName) {
            if (authenticationCount == 1) {
                [securityCredentials setUserName:@"Code Monkey"];
            } else {
                [securityCredentials setUserName:userName];
            }
        }
        if (mask & kAJNSecurityCredentialTypePassword) {
            [securityCredentials setPassword:@"123banana321"];
        }
        return securityCredentials;
    }
    
    return nil;    
}

- (void)authenticationUsing:(NSString*)authenticationMechanism forRemotePeer:(NSString*)peer didCompleteWithStatus:(BOOL)success
{
    NSLog(@"Authentication %@ for peer %@ %@\n", authenticationMechanism, peer, success ? @" was succesful" : @" failed");    
}

- (BOOL)verifySecurityCredentials:(AJNSecurityCredentials*)credentials usingAuthenticationMechanism:(NSString*)authenticationMechanism forRemotePeer:(NSString*)peerName
{
    bool result = false;
    if ([authenticationMechanism compare:@"ALLJOYN_RSA_KEYX"] == NSOrderedSame) {
        if ([credentials isCredentialTypeSet:kAJNSecurityCredentialTypeCertificateChain]) {
            NSLog(@"Verify\n%@\n", credentials.certificateChain);
            result = true;
        }
    }
    return result;        
}

- (void)securityViolationOccurredWithErrorCode:(QStatus)errorCode forMessage:(AJNMessage*)message
{
    NSLog(@"Security violation occurred. %@\n", [AJNStatus descriptionForStatusCode:errorCode]);
}

@end
