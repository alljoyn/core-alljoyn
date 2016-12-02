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

#import <Foundation/Foundation.h>

/**
 * Version and build information for AllJoyn
 */
@interface AJNVersion : NSObject

/**
 * Version information for the AllJoyn library
 */
+ (NSString *)versionInformation;

/**
 * Build information for the AllJoyn library.
 */
+ (NSString *)buildInformation;

/**
 * Version number of the AllJoyn library.
 */
+ (NSInteger)versionNumber;

@end