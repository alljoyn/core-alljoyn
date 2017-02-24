////////////////////////////////////////////////////////////////////////////////
//    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
//    Project (AJOSP) Contributors and others.
//    
//    SPDX-License-Identifier: Apache-2.0
//    
//    All rights reserved. This program and the accompanying materials are
//    made available under the terms of the Apache License, Version 2.0
//    which accompanies this distribution, and is available at
//    http://www.apache.org/licenses/LICENSE-2.0
//    
//    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
//    Alliance. All rights reserved.
//    
//    Permission to use, copy, modify, and/or distribute this software for
//    any purpose with or without fee is hereby granted, provided that the
//    above copyright notice and this permission notice appear in all
//    copies.
//    
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
//    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
//    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
//    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
//    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
//    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
//    PERFORMANCE OF THIS SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#import <XCTest/XCTest.h>
#import "AllJoynCodeGenerator.h"

@interface AllJoynCodeGeneratorTests : XCTestCase

@property (nonatomic) AllJoynCodeGenerator *codeGenerator;

@end

@implementation AllJoynCodeGeneratorTests

- (void)setUp {
    [super setUp];

    _codeGenerator = [[AllJoynCodeGenerator alloc] init];
}

- (void)tearDown {
    _codeGenerator = nil;

    [super tearDown];
}

- (void)testWrongNumberOfArguments {
    AJCGError code;

    code = [_codeGenerator runWithArgc:0 argv:NULL];
    XCTAssert(code == AJCG_UnexpectedArguments);

    const char *argsSet1[] = {"appname", "file1"};
    code = [_codeGenerator runWithArgc:sizeof(argsSet1) / sizeof(argsSet1[0]) argv:argsSet1];
    XCTAssert(code == AJCG_UnexpectedArguments);

    const char *argsSet2[] = {"appname", "file1", "file2", "file3"};
    code = [_codeGenerator runWithArgc:sizeof(argsSet2) / sizeof(argsSet2[0]) argv:argsSet2];
    XCTAssert(code == AJCG_UnexpectedArguments);
}

- (void)testWrongArguments {
    NSString *path = [self currentDir];
    NSString *caseDir = @"BasicObjectCase";
    NSString *appPath = [NSString stringWithFormat:@"%@/appname", path];
    AJCGError code;

    const char *argsSet1[] = {[appPath UTF8String], "WrongXMLFile", "BasicObject"};
    code = [_codeGenerator runWithArgc:sizeof(argsSet1) / sizeof(argsSet1[0]) argv:argsSet1];
    XCTAssert(code == AJCG_BadXMLFile);

    NSString *xmlPath = [NSString stringWithFormat:@"%@/%@/BasicObjectModel.xml", path, caseDir];
    const char *argsSet2[] = {[appPath UTF8String], [xmlPath UTF8String], "Basic/Object"};
    code = [_codeGenerator runWithArgc:sizeof(argsSet2) / sizeof(argsSet2[0]) argv:argsSet2];
    XCTAssert(code == AJCG_BadBaseFileName);
}

- (NSString *)currentDir {
    return [[[NSBundle bundleForClass:[self class]] bundlePath] stringByDeletingLastPathComponent];
}

- (void)testBasicObject {
    NSString *path = [self currentDir];
    NSString *caseDir = [NSString stringWithFormat:@"%@/BasicObjectCase", path];
    NSString *baseFileName = @"BasicObject";

    // Generete AllJoyn Object ObjC sources from XML description
    //
    NSString *xmlPath = [NSString stringWithFormat:@"%@/BasicObjectModel.xml", caseDir];
    AJCGError code = [_codeGenerator runWithExecutableDirectory:path inputXMLFile:xmlPath baseFileName:baseFileName];
    XCTAssert(code == AJCG_Ok);

    // Compate generated AJNBasicObject.h with reference
    //
    NSString *ajnObjectH_Path = [NSString stringWithFormat:@"%@/AJN%@.h", caseDir, baseFileName];
    NSString *ajnObjectH_Content = [NSString stringWithContentsOfFile:ajnObjectH_Path encoding:NSUTF8StringEncoding error:NULL];
    XCTAssert(ajnObjectH_Content != nil);

    NSString *ajnObjectRefH_Path = [NSString stringWithFormat:@"%@/AJN%@Ref.h", caseDir, baseFileName];
    NSString *ajnObjectRefH_Content = [NSString stringWithContentsOfFile:ajnObjectRefH_Path encoding:NSUTF8StringEncoding error:NULL];
    XCTAssert(ajnObjectRefH_Content != nil);

    XCTAssert([ajnObjectH_Content compare:ajnObjectRefH_Content] == NSOrderedSame);

    // Compate generated AJNBasicObject.mm with reference
    //
    NSString *ajnObjectMM_Path = [NSString stringWithFormat:@"%@/AJN%@.mm", caseDir, baseFileName];
    NSString *ajnObjectMM_Content = [NSString stringWithContentsOfFile:ajnObjectMM_Path encoding:NSUTF8StringEncoding error:NULL];
    XCTAssert(ajnObjectMM_Content != nil);

    NSString *ajnObjectRefMM_Path = [NSString stringWithFormat:@"%@/AJN%@Ref.mm", caseDir, baseFileName];
    NSString *ajnObjectRefMM_Content = [NSString stringWithContentsOfFile:ajnObjectRefMM_Path encoding:NSUTF8StringEncoding error:NULL];
    XCTAssert(ajnObjectRefMM_Content != nil);

    XCTAssert([ajnObjectMM_Content compare:ajnObjectRefMM_Content] == NSOrderedSame);

    // Compate generated BasicObject.h with reference
    //
    NSString *objectH_Path = [NSString stringWithFormat:@"%@/%@.h", caseDir, baseFileName];
    NSString *objectH_Content = [NSString stringWithContentsOfFile:objectH_Path encoding:NSUTF8StringEncoding error:NULL];
    XCTAssert(objectH_Content != nil);

    NSString *objectRefH_Path = [NSString stringWithFormat:@"%@/%@Ref.h", caseDir, baseFileName];
    NSString *objectRefH_Content = [NSString stringWithContentsOfFile:objectRefH_Path encoding:NSUTF8StringEncoding error:NULL];
    XCTAssert(objectRefH_Content != nil);

    XCTAssert([objectH_Content compare:objectRefH_Content] == NSOrderedSame);

    // Compate generated BasicObject.m with reference
    //
    NSString *objectM_Path = [NSString stringWithFormat:@"%@/%@.m", caseDir, baseFileName];
    NSString *objectM_Content = [NSString stringWithContentsOfFile:objectM_Path encoding:NSUTF8StringEncoding error:NULL];
    XCTAssert(objectM_Content != nil);

    NSString *objectRefM_Path = [NSString stringWithFormat:@"%@/%@Ref.m", caseDir, baseFileName];
    NSString *objectRefM_Content = [NSString stringWithContentsOfFile:objectRefM_Path encoding:NSUTF8StringEncoding error:NULL];
    XCTAssert(objectRefM_Content != nil);

    XCTAssert([objectM_Content compare:objectRefM_Content] == NSOrderedSame);
}

@end