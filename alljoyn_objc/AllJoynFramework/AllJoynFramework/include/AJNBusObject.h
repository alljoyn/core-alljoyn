////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
//    Source Project (AJOSP) Contributors and others.
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
//     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
//     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
//     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
//     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
//     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
//     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
//     PERFORMANCE OF THIS SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#import <Foundation/Foundation.h>
#import "AJNObject.h"
#import "AJNStatus.h"
#import "AJNSessionOptions.h"
#import "AJNInterfaceMember.h"
#import "AJNMessage.h"
#import "AJNTranslator.h"

@class AJNBusAttachment;
@class AJNInterfaceDescription;
@class AJNMessageArgument;

/**
 * Message Bus Object base protocol. All application bus object protocols should inherit this.
 */
@protocol AJNBusObject<AJNHandle>

/**
 * Return the path for the object
 *
 * @return Object path
 */
@property (nonatomic, readonly) NSString *path;

/**
 * Get the name of this object.
 * The name is the last component of the path.
 *
 * @return Last component of object path.
 */
@property (nonatomic, readonly) NSString *name;

/**
 * Get a list of the interfaces that are added to this BusObject that will be announced.
 */
@property (nonatomic, readonly) NSArray<NSString *> *announcedInterfaceNames;

/**
 * Indicates if this object is secure.
 *
 * @return Return true if authentication is required to emit signals or call methods on this object.
 */
@property (nonatomic, readonly) BOOL isSecure;

/**
 * Flag used to specify if an interface is announced or not here
 * or Alternatively if one uses SetAnnounceFlag function
 * @see AddInterface
 * @see SetAnnounceFlag
 */
typedef enum AJNAnnounceFlag {
    UNANNOUNCED=0,
    ANNOUNCED=1
} AJNAnnounceFlag;

/**
 * AJNBusObject initialization.
 *
 * @param busAttachment  Bus that this object exists on.
 * @param path           Object path for object.
 */
- (id)initWithBusAttachment:(AJNBusAttachment *)busAttachment onPath:(NSString *)path;

/**
 * Called by the message bus when the object has been successfully registered. The object can
 * perform any initialization such as adding match rules at this time.
 */
- (void)objectWasRegistered;

/**
 * Emit PropertiesChanged to signal the bus that this property has been updated
 *
 *
 * @param propertyName  The name of the property being changed
 * @param interfaceName The name of the interface
 * @param value         The new value of the property
 * @param sessionId     ID of the session we broadcast the signal to (0 for all)
 */
- (void)emitPropertyWithName:(NSString *)propertyName onInterfaceWithName:(NSString *)interfaceName changedToValue:(AJNMessageArgument *)value inSession:(AJNSessionId)sessionId;

/**
 * Emit PropertiesChanged to signal the bus that this property has been updated
 *
 *
 * @param propertyName  The name of the property being changed
 * @param interfaceName The name of the interface
 * @param value         The new value of the property
 * @param sessionId     ID of the session we broadcast the signal to (0 for all)
 * @param flags         Flags to be added to the signal.
 */
- (void)emitPropertyWithName:(NSString *)propertyName onInterfaceWithName:(NSString *)interfaceName changedToValue:(AJNMessageArgument *)value inSession:(AJNSessionId)sessionId withFlags:(uint8_t)flags;

/**
 * Emit PropertiesChanged to signal the bus that these properties have been updated
 *
 *  BusObject must be registered before calling this method.
 *
 * @param propNames An array with the names of the properties being changed
 * @param ifcName   The name of the interface
 * @param sessionId ID of the session we broadcast the signal to (0 for all)
 * @param flags     Flags to be added to the signal.
 *
 * @return   ER_OK if successful.
 */
- (QStatus)emitPropertiesWithNames:(NSArray *)propNames onInterfaceWithName:(NSString *)ifcName inSession:(AJNSessionId)sessionId withFlags:(uint8_t)flags;

/**
 * Send a signal.
 *
 * When using session-cast signals in a multi-point session, all members of
 * the session will see the signal.
 *
 * When using security and session-cast signals in a multipoint session all
 * members must be in an established trust relationship or a specific
 * destination specified. Otherwise the signal will not been seen by any peers.
 *
 * When using security with policy and manifest (aka security 2.0) if the
 * destination is not specified only the receiving peers policy will be used
 * when deciding to trust the signal. The sending peer will not check its
 * policy before sending the signal.
 *
 * There is no way to securely transmit sessionless signals since there is
 * no way to establish a trust relationship between sending and receiving
 * peers
 *
 * @param destination  The unique or well-known bus name or the signal recipient (NULL for broadcast signals)
 * @param sessionId    A unique SessionId for this AllJoyn session instance. The session this message is for.
 *                     Use SESSION_ID_ALL_HOSTED to emit on all sessions hosted by this BusObject's BusAttachment.
 *                     For broadcast or sessionless signals, the sessionId must be 0.
 * @param signal       Interface member of signal being emitted.
 *
 * @return
 *      - ER_OK if successful
 *      - ER_BUS_OBJECT_NOT_REGISTERED if bus object has not yet been registered
 *      - An error status otherwise
 */
- (QStatus)signal:(NSString *)destination inSession:(AJNSessionId)sessionId withSignal:(AJNInterfaceMember *)signal;

/**
 * Send a signal.
 *
 * When using session-cast signals in a multi-point session, all members of
 * the session will see the signal.
 *
 * When using security and session-cast signals in a multipoint session all
 * members must be in an established trust relationship or a specific
 * destination specified. Otherwise the signal will not been seen by any peers.
 *
 * When using security with policy and manifest (aka security 2.0) if the
 * destination is not specified only the receiving peers policy will be used
 * when deciding to trust the signal. The sending peer will not check its
 * policy before sending the signal.
 *
 * There is no way to securely transmit sessionless signals since there is
 * no way to establish a trust relationship between sending and receiving
 * peers
 *
 * @param destination  The unique or well-known bus name or the signal recipient (NULL for broadcast signals)
 * @param sessionId    A unique SessionId for this AllJoyn session instance. The session this message is for.
 *                     Use SESSION_ID_ALL_HOSTED to emit on all sessions hosted by this BusObject's BusAttachment.
 *                     For broadcast or sessionless signals, the sessionId must be 0.
 * @param signal       Interface member of signal being emitted.
 * @param args         The arguments for the signal (can be NULL)
 *
 * @return
 *      - ER_OK if successful
 *      - ER_BUS_OBJECT_NOT_REGISTERED if bus object has not yet been registered
 *      - An error status otherwise
 */
- (QStatus)signal:(NSString *)destination inSession:(AJNSessionId)sessionId withSignal:(AJNInterfaceMember *)signal withArgs:(NSArray *)args;

/**
 * Send a signal.
 *
 * When using session-cast signals in a multi-point session, all members of
 * the session will see the signal.
 *
 * When using security and session-cast signals in a multipoint session all
 * members must be in an established trust relationship or a specific
 * destination specified. Otherwise the signal will not been seen by any peers.
 *
 * When using security with policy and manifest (aka security 2.0) if the
 * destination is not specified only the receiving peers policy will be used
 * when deciding to trust the signal. The sending peer will not check its
 * policy before sending the signal.
 *
 * There is no way to securely transmit sessionless signals since there is
 * no way to establish a trust relationship between sending and receiving
 * peers
 *
 * @param destination  The unique or well-known bus name or the signal recipient (NULL for broadcast signals)
 * @param sessionId    A unique SessionId for this AllJoyn session instance. The session this message is for.
 *                     Use SESSION_ID_ALL_HOSTED to emit on all sessions hosted by this BusObject's BusAttachment.
 *                     For broadcast or sessionless signals, the sessionId must be 0.
 * @param signal       Interface member of signal being emitted.
 * @param args         The arguments for the signal (can be NULL)
 * @param timeToLive   If non-zero this specifies the useful lifetime for this signal.
 *                     For sessionless signals the units are seconds.
 *                     For all other signals the units are milliseconds.
 *                     If delivery of the signal is delayed beyond the timeToLive due to
 *                     network congestion or other factors the signal may be discarded. There is
 *                     no guarantee that expired signals will not still be delivered.
 * @param flags        Logical OR of the message flags for this signals. The following flags apply to signals:
 *                     - If ::ALLJOYN_FLAG_GLOBAL_BROADCAST is set broadcast signal (null destination) will be
 *                       forwarded to all Routing Nodes in the system.
 *                     - If ::ALLJOYN_FLAG_ENCRYPTED is set the message is authenticated and the payload if any is
 *                       encrypted.
 *                     - If ::ALLJOYN_FLAG_SESSIONLESS is set the signal will be sent as a Sessionless Signal. NOTE:
 *                       if this flag and the GLOBAL_BROADCAST flags are set it could result in the same signal
 *                       being received twice.
 *
 * @param msg          [OUT] If non-null, the sent signal message is returned to the caller.
 * @return
 *      - ER_OK if successful
 *      - ER_BUS_OBJECT_NOT_REGISTERED if bus object has not yet been registered
 *      - An error status otherwise
 */
- (QStatus)signal:(NSString *)destination inSession:(AJNSessionId)sessionId withSignal:(AJNInterfaceMember *)signal withArgs:(NSArray *)args ttl:(uint16_t)timeToLive withFlags:(uint8_t)flags withMsg:(AJNMessage **)msg;

/**
 * Remove sessionless message sent from this object from local daemon's
 * store/forward cache.
 *
 * @param serialNumber    Serial number of previously sent sessionless signal.
 * @return   ER_OK if successful.
 */
- (QStatus)cancelSessionlessMessageWithSerial:(uint32_t)serialNumber;

/**
 * Remove sessionless message sent from this object from local daemon's
 * store/forward cache.
 *
 * @param message    Message to be removed.
 * @return   ER_OK if successful.
 */
- (QStatus)cancelSessionlessMessageWithMessage:(const AJNMessage *)message;

/**
 * Change the announce flag for an already added interface. Changes in the
 * announce flag are not visible to other devices till Announce is called.
 *
 * @see AboutObj::Announce()
 *
 * @param[in] iface InterfaceDescription for the interface you wish to set
 *                  the the announce flag.
 * @param[in] isAnnounced This interface should be part of the Announce signal
 *                        UNANNOUNCED - this interface will not be part of the Announce
 *                                      signal
 *                        ANNOUNCED - this interface will be part of the Announce
 *                                    signal.
 * @return
 *  - ER_OK if successful
 *  - ER_BUS_OBJECT_NO_SUCH_INTERFACE if the interface is not part of the
 *                                     bus object.
 */
- (QStatus)setAnnounceFlagForInterface:(AJNInterfaceDescription *)iface value:(AJNAnnounceFlag)isAnnounced;

@end

////////////////////////////////////////////////////////////////////////////////

/**
 * Message Bus Object base class.
 */
@interface AJNBusObject : AJNObject<AJNBusObject>

@property (nonatomic, readonly) NSString *path;

@property (nonatomic, readonly) NSString *name;

@property (nonatomic, readonly) BOOL isSecure;

@property (nonatomic) void *translator;


- (id)initWithPath:(NSString *)path;

- (id)initWithBusAttachment:(AJNBusAttachment *)busAttachment onPath:(NSString *)path;

- (void)objectWasRegistered;

- (void)setDescription:(NSString *)description inLanguage:(NSString *)language;

- (void)setDescriptionTranslator:(id<AJNTranslator>)translator;

- (QStatus)setAnnounceFlagForInterface:(AJNInterfaceDescription *)iface value:(AJNAnnounceFlag)flag;

@end