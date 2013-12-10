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

#import "AJNObject.h"
#import "AJNInterfaceMember.h"
#import "AJNMessageHeaderFields.h"

/** @name Flag types */
// @{
typedef uint8_t AJNMessageFlag;

/** No reply is expected*/
extern const AJNMessageFlag kAJNMessageFlagNoReplyExpected;
/** Auto start the service */
extern const AJNMessageFlag kAJNMessageFlagAutoStart;
/** Allow messages from remote hosts (valid only in Hello message) */
extern const AJNMessageFlag kAJNMessageFlagAllowRemoteMessages;
/** Sessionless message  */
extern const AJNMessageFlag kAJNMessageFlagSessionless;
/** Global (bus-to-bus) broadcast */
extern const AJNMessageFlag kAJNMessageFlagGlobalBroadcast;
/** Header is compressed */
extern const AJNMessageFlag kAJNMessageFlagCompressed;
/** Body is encrypted */
extern const AJNMessageFlag kAJNMessageFlagEncrypted;
// @}

/**
 * Class that represents a message sent on the AllJoyn bus
 */
@interface AJNMessage : AJNObject

/**
 * Determine if message is a broadcast signal.
 *
 * @return  Return true if this is a broadcast signal.
 */
@property (nonatomic, readonly) BOOL isBroadcastSignal;

/**
 * Messages broadcast to all devices are global broadcast messages.
 *
 * @return  Return true if this is a global broadcast message.
 */
@property (nonatomic, readonly) BOOL isGlobalBroadcast;

/**
 * Returns the flags for the message.
 * @return flags for the message
 *
 */
@property (nonatomic, readonly) AJNMessageFlag flags;

/**
 * Return true if message's TTL header indicates that it is expired
 */
@property (nonatomic, readonly) BOOL isExpired;

/**
 *
 * Returns number of milliseconds before message expires if non-null
 * If message never expires value is set to the maximum uint32_t value.
 *
 * @return number of milliseconds before message expires.
 */
@property (nonatomic, readonly) uint32_t timeUntilExpiration;

/**
 * Determine if the message is marked as unreliable. Unreliable messages have a non-zero
 * time-to-live and may be silently discarded.
 *
 * @return  Returns true if the message is unreliable, that is, has a non-zero time-to-live.
 */
@property (nonatomic, readonly) BOOL isUnreliable;

/**
 * Determine if the message was encrypted.
 *
 * @return  Returns true if the message was encrypted.
 */
@property (nonatomic, readonly) BOOL isEncrypted;

/**
 * Get the name of the authentication mechanism that was used to generate the encryption key if
 * the message is encrypted.
 *
 * @return  the name of an authentication mechanism or an empty string.
 */
@property (nonatomic, readonly) NSString *authenticationMechanism;

/**
 * Return the type of the message
 */
@property (nonatomic, readonly) AJNMessageType type;

/**
 * Return the arguments for this message.
 *
 * @return Returns the message arguments in an array. The array is populated with instances of the AJNMessageArgument class.
 */
@property (nonatomic, readonly) NSArray *arguments;

/**
 * Accessor function to get serial number for the message. Usually only important for
 * method calls and it is used for matching up the reply to the call.
 *
 * @return the serial number of the %Message
 */
@property (nonatomic, readonly) uint32_t callSerialNumber;

/**
 * Accessor function to get the reply serial number for the message. Only meaningful for method reply types.
 * @return  - The serial number for the message stored in the AllJoyn header field
 *          - Zero if unable to find the serial number. Note that 0 is an invalid serial number.
 */
@property (nonatomic, readonly) uint32_t replySerialNumber;

/**
 * Get a reference to all of the header fields for this message.
 *
 * @return A reference to the header fields for this message.
 */
@property (nonatomic, readonly) AJNMessageHeaderFields *headerFields;

/**
 * Property containing the signature for this message
 * @return  - The AllJoyn SIGNATURE string stored in the AllJoyn header field
 *          - An empty string if unable to find the AllJoyn signature
 */
@property (nonatomic, readonly) NSString *signature;

/**
 * Property containing the object path for this message
 * @return  - The AllJoyn object path string stored in the AllJoyn header field
 *          - An empty string if unable to find the AllJoyn object path
 */
@property (nonatomic, readonly) NSString *objectPath;

/**
 * Property containing the interface for this message
 * @return  - The AllJoyn interface string stored in the AllJoyn header field
 *          - An empty string if unable to find the interface
 */
@property (nonatomic, readonly) NSString *interfaceName;

/**
 * Accessor function to get the member (method/signal) name for this message
 * @return  - The AllJoyn member (method/signal) name string stored in the AllJoyn header field
 *          - An empty string if unable to find the member name
 */
@property (nonatomic, readonly) NSString *memberName;

/**
 * Accessor function to get the sender for this message.
 *
 * @return  - The senders well-known name string stored in the AllJoyn header field.
 *          - An empty string if the message did not specify a sender.
 */
@property (nonatomic, readonly) NSString *senderName;

/**
 * Get the unique name of the endpoint that the message was received on.
 *
 * @return  - The unique name of the endpoint that the message was received on.
 */
@property (nonatomic, readonly) NSString *receiverEndpointName;

/**
 * Accessor function to get the destination for this message
 * @return  - The message destination string stored in the AllJoyn header field.
 *          - An empty string if unable to find the message destination.
 */
@property (nonatomic, readonly) NSString *destination;

/**
 * Accessor function to get the compression token for the message.
 *
 * @return  - Compression token for the message stored in the AllJoyn header field
 *          - 0 'zero' if there is no compression token.
 */
@property (nonatomic, readonly) uint32_t compressionToken;

/**
 * Accessor function to get the session id for the message.
 *
 * @return  - Session id for the message
 *          - 0 'zero' if sender did not specify a session
 */
@property (nonatomic, readonly) uint32_t sessionId;

/**
 * If the message is an error message returns the error name and optionally the error message string
 * @return  - If the message is an error message return the error name from the AllJoyn header
 *          - NULL if the message type is not MESSAGE_ERROR.
 */
@property (nonatomic, readonly) NSString *errorName;

/**
 * Returns a complete description of an error by concatenating the error name and the error
 * message together.
 *
 * @return  - If the message is an error message return the error description
 *          - An empty string if the message type is not MESSAGE_ERROR.
 */
@property (nonatomic, readonly) NSString *errorDescription;

/**
 * In debug builds returns a string that provides a brief description of the message. In release
 * builds returns and empty string.
 *
 * @return a brief description of the message or an empty string.
 */
@property (nonatomic, readonly) NSString *description;

/**
 * In debug builds returns an XML string representation of the message. In release builds
 * returns an empty string.
 *
 * @return an XML string representation of the message or an empty string.
 */
@property (nonatomic, readonly) NSString *xmlDescription;

/**
 * Returns the timestamp (in milliseconds) for this message. If the message header contained a
 * timestamp this is the estimated timestamp for when the message was sent by the remote device,
 * otherwise it is the timestamp for when the message was unmarshaled. Note that the timestamp
 * is always relative to local time.
 *
 * @return The timestamp for this message.
 */
@property (nonatomic, readonly) uint32_t timeStamp;

@end
