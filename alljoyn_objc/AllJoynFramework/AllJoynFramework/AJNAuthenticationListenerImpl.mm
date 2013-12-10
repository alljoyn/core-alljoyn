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

#import "AJNAuthenticationListenerImpl.h"

@interface AJNSecurityCredentials(Private)

/** Helper to access the underlying AllJoyn C++ API object that this objective-c class encapsulates
 */
- (ajn::AuthListener::Credentials*)credentials;

@end

/**
 * Constructor for the AJN authentication listener implementation.
 *
 * @param aDelegate         Objective C delegate called when one of the below virtual functions is called.     
 */    
AJNAuthenticationListenerImpl::AJNAuthenticationListenerImpl(id<AJNAuthenticationListener> aDelegate) : m_delegate(aDelegate)
{
    
}

/**
 * Virtual destructor for derivable class.
 */
AJNAuthenticationListenerImpl::~AJNAuthenticationListenerImpl()
{
    m_delegate = nil;
}

/**
 * Authentication mechanism requests user credentials. If the user name is not an empty string
 * the request is for credentials for that specific user. A count allows the listener to decide
 * whether to allow or reject multiple authentication attempts to the same peer.
 *
 * @param authMechanism  The name of the authentication mechanism issuing the request.
 * @param peerName       The name of the remote peer being authenticated.  On the initiating
 *                       side this will be a well-known-name for the remote peer. On the
 *                       accepting side this will be the unique bus name for the remote peer.
 * @param authCount      Count (starting at 1) of the number of authentication request attempts made.
 * @param userName       The user name for the credentials being requested.
 * @param credMask       A bit mask identifying the credentials being requested. The application
 *                       may return none, some or all of the requested credentials.
 * @param[out] credentials    The credentials returned.
 *
 * @return  The caller should return true if the request is being accepted or false if the
 *          requests is being rejected. If the request is rejected the authentication is
 *          complete.
 */
bool AJNAuthenticationListenerImpl::RequestCredentials(const char* authMechanism, const char* peerName, uint16_t authCount, const char* userName, uint16_t credMask, Credentials& credentials)
{
    AJNSecurityCredentials *creds = [m_delegate requestSecurityCredentialsWithAuthenticationMechanism:[NSString stringWithCString:authMechanism encoding:NSUTF8StringEncoding] peerName:[NSString stringWithCString:peerName encoding:NSUTF8StringEncoding] authenticationCount:authCount userName:[NSString stringWithCString:userName encoding:NSUTF8StringEncoding] credentialTypeMask:credMask];
    if (creds) {
        credentials = *(creds.credentials);
    }
    return creds != nil;
}

/**
 * Authentication mechanism requests verification of credentials from a remote peer.
 *
 * @param authMechanism  The name of the authentication mechanism issuing the request.
 * @param peerName       The name of the remote peer being authenticated.  On the initiating
 *                       side this will be a well-known-name for the remote peer. On the
 *                       accepting side this will be the unique bus name for the remote peer.
 * @param credentials    The credentials to be verified.
 *
 * @return  The listener should return true if the credentials are acceptable or false if the
 *          credentials are being rejected.
 */
bool AJNAuthenticationListenerImpl::VerifyCredentials(const char* authMechanism, const char* peerName, const Credentials& credentials)
{
    bool result = true;
    if ([m_delegate respondsToSelector:@selector(verifySecurityCredentials:usingAuthenticationMechanism:forRemotePeer:)]) {
        result = [m_delegate verifySecurityCredentials:[[AJNSecurityCredentials alloc] initWithHandle:(AJNHandle)(&credentials)] usingAuthenticationMechanism:[NSString stringWithCString:authMechanism encoding:NSUTF8StringEncoding ] forRemotePeer:[NSString stringWithCString:peerName encoding:NSUTF8StringEncoding]];
    }
    return result;
}

/**
 * Optional method that if implemented allows an application to monitor security violations. This
 * function is called when an attempt to decrypt an encrypted messages failed or when an unencrypted
 * message was received on an interface that requires encryption. The message contains only
 * header information.
 *
 * @param status  A status code indicating the type of security violation.
 * @param msg     The message that cause the security violation.
 */
void AJNAuthenticationListenerImpl::SecurityViolation(QStatus status, const ajn::Message& msg)
{
    if ([m_delegate respondsToSelector:@selector(securityViolationOccurredWithErrorCode:forMessage:)]) {
        [m_delegate securityViolationOccurredWithErrorCode:status forMessage:[[AJNMessage alloc] initWithHandle:(AJNHandle)(&msg)]];
    }
}

/**
 * Reports successful or unsuccessful completion of authentication.
 *
 * @param authMechanism  The name of the authentication mechanism that was used or an empty
 *                       string if the authentication failed.
 * @param peerName       The name of the remote peer being authenticated.  On the initiating
 *                       side this will be a well-known-name for the remote peer. On the
 *                       accepting side this will be the unique bus name for the remote peer.
 * @param success        true if the authentication was successful, otherwise false.
 */
void AJNAuthenticationListenerImpl::AuthenticationComplete(const char* authMechanism, const char* peerName, bool success)
{
    [m_delegate authenticationUsing:[NSString stringWithCString:authMechanism encoding:NSUTF8StringEncoding] forRemotePeer:[NSString stringWithCString:peerName encoding:NSUTF8StringEncoding] didCompleteWithStatus:success == true];
}
