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

#import <alljoyn/InterfaceDescription.h>
#import "AJNInterfaceProperty.h"

@implementation AJNInterfaceProperty

/**
 * Helper to return the C++ API object that is encapsulated by this objective-c class
 */
- (ajn::InterfaceDescription::Property*)property
{
    return static_cast<ajn::InterfaceDescription::Property*>(self.handle);
}

- (NSString*)name
{
    return [NSString stringWithCString:self.property->name.c_str() encoding:NSUTF8StringEncoding];
}

- (NSString*)signature
{
    return [NSString stringWithCString:self.property->signature.c_str() encoding:NSUTF8StringEncoding];
}

- (AJNInterfacePropertyAccessPermissionsFlags)accessPermissions
{
    return self.property->access;
}

- (NSString *)annotationWithName:(NSString *)annotationName
{
    qcc::String value;
    bool wasPropertyFound = self.property->GetAnnotation([annotationName UTF8String], value);
    NSString *annotationValue;
    if (wasPropertyFound) {
        annotationValue = [NSString stringWithCString:value.c_str() encoding:NSUTF8StringEncoding];
    }
    return annotationValue;
}


@end