//
//  BasicObjectExtention.m
//  CodeGeneratorCompileTest
//
//  Created by Eugene Komarov on 20/02/2017.
//  Copyright © 2017 AllSeen Alliance. All rights reserved.
//

#import "BasicObjectExtention.h"

@implementation BasicObjectExtention

- (QStatus)concatenateString:(NSString *)str1 withString:(NSString *)str2 outString:(NSString **)outStr message:(AJNMessage *)methodCallMessage
{
    *outStr = [NSString stringWithFormat:@"%@%@",str1, str2];
    return ER_OK;
}

- (QStatus)methodWithInString:(NSString *)str outString1:(NSString **)outStr1 outString2:(NSString **)outStr2 message:(AJNMessage *)methodCallMessage
{
    *outStr1 = [NSString stringWithFormat:@"%@+1",str];
    *outStr2 = [NSString stringWithFormat:@"%@+1",str];
    return ER_OK;
}

- (QStatus)methodWithOutString:(NSString *)str1 inString2:(NSString *)str2 outString1:(NSString **)outStr1 outString2:(NSString **)outStr2 message:(AJNMessage *)methodCallMessage
{
    *outStr1 = [NSString stringWithFormat:@"%@+1",str1];
    *outStr2 = [NSString stringWithFormat:@"%@+1",str2];
    return ER_OK;
}

- (QStatus) methodWithOnlyOutString:(NSString **)outStr1 outString2:(NSString **)outStr2 message:(AJNMessage *)methodCallMessage
{
    *outStr1 = [NSString stringWithFormat:@"1st string"];
    *outStr2 = [NSString stringWithFormat:@"2st string"];
    return ER_OK;
}

- (QStatus)methodWithNoReturnAndNoArgs:(AJNMessage *)methodCallMessage
{
    return ER_OK;
}

- (QStatus) methodWithReturnAndNoInArgs:(NSString **)outStr message:(AJNMessage *)methodCallMessage
{
    *outStr = [NSString stringWithFormat:@"only out string"];
    return ER_OK;
}

- (QStatus)methodWithStringArray:(AJNMessageArgument *)stringArray structWithStringAndInt:(AJNMessageArgument *)aStruct outStr:(NSString **)outStr message:(AJNMessage *)methodCallMessage
{
    // TODO: complete the implementation of this method
    //
    @throw([NSException exceptionWithName:@"NotImplementedException" reason:@"You must implement this method" userInfo:nil]);
}

@end
