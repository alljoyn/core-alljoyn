////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013-2014, AllSeen Alliance. All rights reserved.
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
#import "AJNObject.h"
#import "AJNStatus.h"
#import "AJNSessionOptions.h"
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
typedef enum AJNAnnounceFlag{
    UNANNOUNCED=0,
    ANNOUNCED=1
}AJNAnnounceFlag;

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

- (void)setDescription:(NSString*)description inLanguage:(NSString*)language;

- (void)setDescriptionTranslator:(id<AJNTranslator>)translator;

@end
