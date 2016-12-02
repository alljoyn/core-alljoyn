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
#import <alljoyn/Status.h>

/**
 * Access to status and error codes for AllJoyn
 */
@interface AJNStatus : NSObject

/**
 * Convert a status code to a string.
 *
 * QCC_StatusText(ER_OK) returns the string "ER_OK"
 *
 * @param status    Status code to be converted.
 *
 * @return  string representation of the status code.
 */
+ (NSString *)descriptionForStatusCode:(QStatus)status;

@end