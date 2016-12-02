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