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

/** Property access permissions flag type */
typedef uint8_t AJNInterfacePropertyAccessPermissionsFlags;

/** Read-only property access permissions flag */
static const AJNInterfacePropertyAccessPermissionsFlags kAJNInterfacePropertyAccessReadFlag      = 1;
/** Write-only property access permissions flag */
static const AJNInterfacePropertyAccessPermissionsFlags kAJNInterfacePropertyAccessWriteFlag     = 2;
/** Read-Write property access permissions flag */
static const AJNInterfacePropertyAccessPermissionsFlags kAJNInterfacePropertyAccessReadWriteFlag = 3;

////////////////////////////////////////////////////////////////////////////////

/**
 * A class that contains the metadata for a property of an interface
 */
@interface AJNInterfaceProperty : AJNObject

/** Name of the property */
@property (nonatomic, readonly) NSString *name;

/** Signature of the property */
@property (nonatomic, readonly) NSString *signature;

/** Access permissions flags for the property */
@property (nonatomic, readonly) AJNInterfacePropertyAccessPermissionsFlags accessPermissions;

/**
 * Get an annotation value for the property
 *
 * @param annotationName    Name of annotation
 *
 * @return  - string value of annotation if found
 *          - nil if not found
 */
- (NSString *)annotationWithName:(NSString *)annotationName;

@end
