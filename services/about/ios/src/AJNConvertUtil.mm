/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/

#import "AJNConvertUtil.h"

@implementation AJNConvertUtil

+ (qcc::String)convertNSStringToQCCString:(NSString *)nsstring
{
	return ((qcc::String)[nsstring UTF8String]);
}

+ (NSString *)convertQCCStringtoNSString:(qcc::String)qccstring
{
	return (@(qccstring.c_str()));
}

+ (const char *)convertNSStringToConstChar:(NSString *)nsstring
{
	return([nsstring UTF8String]);
}

+ (NSString *)convertConstCharToNSString:(const char *)constchar
{
	if ((constchar == NULL) || (constchar[0] == 0)) {
		return @"";
	}
	else {
		return(@(constchar));
	}
}

@end