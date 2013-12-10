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

#import <Foundation/Foundation.h>
#import <alljoyn/Status.h>
#import "AJNSecurityCredentials.h"
#import "AJNMessage.h"

/**
 * Protocol to allow authentication mechanisms to interact with the application
 */
@protocol AJNAuthenticationListener <NSObject>

@required

/**
 * Authentication mechanism requests user credentials. If the user name is not an empty string
 * the request is for credentials for that specific user. A count allows the listener to decide
 * whether to allow or reject multiple authentication attempts to the same peer.
 *
 * @param authenticationMechanism   The name of the authentication mechanism issuing the request.
 * @param peerName                  The name of the remote peer being authenticated.  On the initiating side this will be a well-known-name for the remote peer. On the accepting side this will be the unique bus name for the remote peer.
 * @param authenticationCount       Count (starting at 1) of the number of authentication request attempts made.
 * @param userName                  The user name for the credentials being requested.
 * @param mask                      A bit mask identifying the credentials being requested. The application may return none, some or all of the requested credentials.
 *
 * @return  The credentials returned. The caller should return nil if the request is being rejected. If the request is rejected the authentication is complete.
 */
- (AJNSecurityCredentials *)requestSecurityCredentialsWithAuthenticationMechanism:(NSString *)authenticationMechanism peerName:(NSString *)peerName authenticationCount:(uint16_t)authenticationCount userName:(NSString *)userName credentialTypeMask:(AJNSecurityCredentialType)mask;

/**
 * Reports successful or unsuccessful completion of authentication.
 *
 * @param authenticationMechanism   The name of the authentication mechanism that was used or an empty string if the authentication failed.
 * @param peerName                  The name of the remote peer being authenticated.  On the initiating side this will be a well-known-name for the remote peer. On the accepting side this will be the unique bus name for the remote peer.
 * @param success                   true if the authentication was successful, otherwise false.
 */
- (void)authenticationUsing:(NSString *)authenticationMechanism forRemotePeer:(NSString *)peerName didCompleteWithStatus:(BOOL)success;

@optional

/**
 * Authentication mechanism requests verification of credentials from a remote peer.
 *
 * @param authenticationMechanism The name of the authentication mechanism issuing the request.
 * @param peerName      The name of the remote peer being authenticated.  On the initiating side this will be a well-known-name for the remote peer. On the accepting side this will be the unique bus name for the remote peer.
 * @param credentials   The credentials to be verified.
 *
 * @return  The listener should return true if the credentials are acceptable or false if the
 *          credentials are being rejected.
 */
- (BOOL)verifySecurityCredentials:(AJNSecurityCredentials *)credentials usingAuthenticationMechanism:(NSString *)authenticationMechanism forRemotePeer:(NSString *)peerName;

/**
 * Optional method that if implemented allows an application to monitor security violations. This
 * function is called when an attempt to decrypt an encrypted messages failed or when an unencrypted
 * message was received on an interface that requires encryption. The message contains only
 * header information.
 *
 * @param errorCode  A status code indicating the type of security violation.
 * @param message    The message that cause the security violation.
 */
- (void)securityViolationOccurredWithErrorCode:(QStatus)errorCode forMessage:(AJNMessage *)message;

@end
