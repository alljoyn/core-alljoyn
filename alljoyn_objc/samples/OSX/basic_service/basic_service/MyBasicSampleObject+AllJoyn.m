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

#import "MyBasicSampleObject+AllJoyn.h"

@implementation MyBasicSampleObject (AllJoyn)

@dynamic handle;
@dynamic path;
@dynamic name;

- (NSString*)concatenateString:(NSString *)string1 withString:(NSString *)string2
{
    [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"MyBasicSampleObject::concatenateString:withString: was called with \"%@\" and \"%@\".\n", string1, string2]];
    return [NSString stringWithFormat:@"%@%@", string1, string2];
}

@end