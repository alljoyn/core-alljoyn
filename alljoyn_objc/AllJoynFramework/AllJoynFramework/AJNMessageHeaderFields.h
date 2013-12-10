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
