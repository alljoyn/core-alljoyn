////////////////////////////////////////////////////////////////////////////////
// // 
//    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
//    Source Project Contributors and others.
//    
//    All rights reserved. This program and the accompanying materials are
//    made available under the terms of the Apache License, Version 2.0
//    which accompanies this distribution, and is available at
//    http://www.apache.org/licenses/LICENSE-2.0

////////////////////////////////////////////////////////////////////////////////

#import "AJNObject.h"

/**
 * AllJoyn Header field types
 */
typedef enum {

    /* Wire-protocol defined header field types */
    kAJNMessageHeaderFieldTypeInvalid = 0,              ///< an invalid header field type
    kAJNMessageHeaderFieldTypePath,                     ///< an object path header field type
    kAJNMessageHeaderFieldTypeInterface,                ///< a message interface header field type
    kAJNMessageHeaderFieldTypeMember,                   ///< a member (message/signal) name header field type
    kAJNMessageHeaderFieldTypeErrorName,               ///< an error name header field type
    kAJNMessageHeaderFieldTypeReplySerial,             ///< a reply serial number header field type
    kAJNMessageHeaderFieldTypeDestination,              ///< message destination header field type
    kAJNMessageHeaderFieldTypeSender,                   ///< senders well-known name header field type
    kAJNMessageHeaderFieldTypeSignature,                ///< message signature header field type
    kAJNMessageHeaderFieldTypeHandles,                  ///< number of file/socket handles that accompany the message
    /* AllJoyn defined header field types */
    kAJNMessageHeaderFieldTypeTimestamp,                ///< time stamp header field type
    kAJNMessageHeaderFieldTypeTimeToLive,             ///< messages time-to-live header field type
    kAJNMessageHeaderFieldTypeCompressionToken,        ///< message compression token header field type
    kAJNMessageHeaderFieldTypeSessionId,               ///< Session id field type
    kAJNMessageHeaderFieldTypeFieldUnknown                   ///< unknown header field type also used as maximum number of header field types.
} AJNMessageHeaderFieldType;

////////////////////////////////////////////////////////////////////////////////

/**
 * AllJoyn header fields
 */
@interface AJNMessageHeaderFields : AJNObject

/**
 * The values of each header field. Each element in the values array is an AJNMessageArgument.
 */
@property (nonatomic, readonly) NSArray *values;

/**
 * The string representation of the header fields.
 */
- (NSString *)stringValue;

@end