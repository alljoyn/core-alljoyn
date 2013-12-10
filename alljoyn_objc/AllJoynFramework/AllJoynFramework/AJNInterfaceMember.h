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
#import "AJNObject.h"

/** @name Annotation flags */
// @{
typedef uint8_t AJNInterfaceAnnotationFlags;

/**< No reply annotate flag */
static const AJNInterfaceAnnotationFlags kAJNInterfaceAnnotationNoReplyFlag     = 1;
/**< Deprecated annotate flag */
static const AJNInterfaceAnnotationFlags kAJNInterfaceAnnotationDeprecatedFlag  = 2;
// @}

////////////////////////////////////////////////////////////////////////////////

/**
 *  Message type enumeration
 */
typedef enum {

    ///< an invalid message type
    kAJNMessageTypeInvalid          = 0,

    ///< a method call message type
    kAJNMessageTypeMethodCall       = 1,

    ///< a method return message type
    kAJNMessageTypeMethodReturn     = 2,

    ///< an error message type
    kAJNMessageTypeError            = 3,

    ///< a signal message type
    kAJNMessageTypeSignal           = 4

} AJNMessageType;

////////////////////////////////////////////////////////////////////////////////

/**
 * Class representing a member of an interface.
 */
@interface AJNInterfaceMember : AJNObject

/**
 * Type of the member.
 */
@property (nonatomic, readonly) AJNMessageType type;

/**
 * Name of the member.
 */
@property (nonatomic, readonly) NSString *name;

/**
 * Input type signature of the member. This is nil for a signal member.
 */
@property (nonatomic, readonly) NSString *inputSignature;

/**
 * Output type signature of the member.
 */
@property (nonatomic, readonly) NSString *outputSignature;

/**
 * Comma separated list of names of all arguments. This can be nil.
 */
@property (nonatomic, readonly) NSArray *argumentNames;

/**
 * Required permissions to invoke this call.
 */
@property (nonatomic, readonly) NSString *accessPermissions;

/**
 * Get the annotation value for the member
 *
 * @param annotationName    Name of annotation
 *
 * @return  - string value of annotation if found
 *          - nil if not found
 */
- (NSString *)annotationWithName:(NSString *)annotationName;

@end
