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

#import "AllJoynCodeGenerator.h"

// constants
//
NSString * const kImplementationHeaderXSL = @"objcHeader.xsl";
NSString * const kImplementationSourceXSL = @"objcSource.xsl";
NSString * const kObjectiveCHeaderXSL = @"objcExtensionHeader.xsl";
NSString * const kObjectiveCSourceXSL = @"objcExtensionSource.xsl";

const NSInteger kExpectedArgumentCount = 2;

static void printToFile(FILE *fd, NSString *format, va_list argList) {
    NSString *string = [[NSString alloc] initWithFormat:format arguments:argList];

    fprintf(stdout, "%s\n", [string UTF8String]);
}

static void print(NSString *format, ...) {
    va_list args;
    va_start(args, format);

    printToFile(stdout, format, args);

    va_end(args);
}

static void printError(NSString *format, ...) {
    va_list args;
    va_start(args, format);

    printToFile(stderr, format, args);

    va_end(args);
}

@implementation NSString (Trimming)

- (NSString *)stringByTrimmingTrailingCharactersInSet:(NSCharacterSet *)characterSet {

    NSUInteger length = [self length];
    
    for (; length > 0; --length) {
        if (![characterSet characterIsMember:[self characterAtIndex:length - 1]]) {
            break;
        }
    }

    return [self substringWithRange:NSMakeRange(0, length)];
}

@end

@implementation NSData (Trimming)

- (NSData *)removeTrailingWhitespacesFromText {

    NSMutableString *result = [[NSMutableString alloc] initWithCapacity:self.length];
    NSString *text = [[NSString alloc] initWithData:self encoding:NSUTF8StringEncoding];

    [text enumerateLinesUsingBlock:^(NSString *line, BOOL *stop) {
        NSString *trimmedLine = [line stringByTrimmingTrailingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];

        [result appendString:trimmedLine];
        [result appendString:@"\n"];
    }];

    return [result dataUsingEncoding:NSUTF8StringEncoding];
}

@end

@implementation AllJoynCodeGenerator

- (void)usage {
    print(@"Usage: AllJoynCodeGenerator [input file] [base file name]");
    print(@"Arguments:");
    print(@"   input file            = The path to the input XML file containing the DBus-formatted object model");
    print(@"   base file name        = The base file name used for the generated Objective-C files");
    print(@"");
    print(@"The files containing the generated Objective-C source code will reside in the same directory as the input file specified.");
    print(@"The xsl files used to generate the Objective-C source code should reside in the same directory as the AllJoynCodeGenerator executable.");
}

- (AJCGError)runWithArgc:(int)argc argv:(const char *[])argv {

    //  Validate the there are the correct number of command line arguments.
    //  Remember that argv/argc includes the executable's path plus the
    //  arguments passed to it.
    //
    if (argc != kExpectedArgumentCount + 1) {
        // Print usage if not enough command line arguments.
        //
        [self usage];

        return AJCG_UnexpectedArguments;
    }

    NSURL *executableFileUrl = [NSURL fileURLWithPath:[NSString stringWithCString:argv[0] encoding:NSUTF8StringEncoding]];
    NSString *executableDirectory = [[executableFileUrl URLByDeletingLastPathComponent] path];
    NSString *xmlFilePath = [NSString stringWithCString:argv[1] encoding:NSUTF8StringEncoding];
    NSString *baseFileName = [NSString stringWithCString:argv[2] encoding:NSUTF8StringEncoding];

    return [self runWithExecutableDirectory:executableDirectory inputXMLFile:xmlFilePath baseFileName:baseFileName];
}

- (AJCGError)runWithExecutableDirectory:(NSString *)executableDirectory inputXMLFile:(NSString *)xmlFilePath baseFileName:(NSString *)baseFileName {

    //  Validate that the input file exists.
    //
    if (![[NSFileManager defaultManager] fileExistsAtPath:xmlFilePath]) {
        printError(@"The input file specified does not exist %@", xmlFilePath);
        return AJCG_BadXMLFile;
    }

    // Validate that the base file name uses valid characters
    //

    NSMutableCharacterSet *validFileNameCharSet = [NSMutableCharacterSet letterCharacterSet];
    [validFileNameCharSet formUnionWithCharacterSet:[NSCharacterSet decimalDigitCharacterSet]];

    if ([baseFileName rangeOfCharacterFromSet:[validFileNameCharSet invertedSet]].location != NSNotFound) {
        printError(@"The base file name should contain only letters or numbers.");
        return AJCG_BadBaseFileName;
    }

    // Parse out the directory of the input file for use as the location of the output files
    //
    NSURL *fileUrl = [NSURL fileURLWithPath:xmlFilePath];
    NSString *outputDirectory = [[fileUrl URLByDeletingLastPathComponent] path];
    NSError *error = nil;
    NSString *xslFileUrl = [NSString stringWithFormat:@"%@/%@", executableDirectory, kImplementationHeaderXSL];
    NSString *outputFilePath = [NSString stringWithFormat:@"%@/AJN%@.h", outputDirectory, baseFileName];
    NSString *outputFileName = [[NSURL fileURLWithPath:outputFilePath] lastPathComponent];

    NSXMLDocument *xmlDocument = [[NSXMLDocument alloc] initWithContentsOfURL:fileUrl options:NSDataReadingMappedIfSafe error:&error];
    if (error) {
        printError(@"Error loading XML. %@", error);
        return AJCG_BadXMLFile;
    }

    // STEP ONE
    //
    // prepare to generate the Objective-C scaffold implementation header file
    //
    NSData *generatedCodeData = [xmlDocument objectByApplyingXSLTAtURL:[NSURL fileURLWithPath:xslFileUrl]
                                                             arguments:@{@"fileName": [NSString stringWithFormat:@"\'%@\'", outputFileName]}
                                                                 error:&error];
    if (error) {
        printError(@"Error applying XSL. %@", error);
        return AJCG_BadXMLFile;
    }

    // write the Objective-C scaffold header file to disk
    //
    generatedCodeData = [generatedCodeData removeTrailingWhitespacesFromText];
    [generatedCodeData writeToURL:[NSURL fileURLWithPath:outputFilePath] atomically:NO];

    // STEP TWO
    //
    // prepare to generate the Objective-C++ scaffold implementation source file
    //
    xslFileUrl = [NSString stringWithFormat:@"%@/%@", executableDirectory, kImplementationSourceXSL];
    outputFilePath = [NSString stringWithFormat:@"%@/AJN%@.mm", outputDirectory, baseFileName];
    outputFileName = [[NSURL fileURLWithPath:outputFilePath] lastPathComponent];

    generatedCodeData = [xmlDocument objectByApplyingXSLTAtURL:[NSURL fileURLWithPath:xslFileUrl]
                                                     arguments:@{@"fileName": [NSString stringWithFormat:@"\'%@\'", outputFileName],
                                                                 @"baseFileName": [NSString stringWithFormat:@"\'%@\'", baseFileName]}
                                                         error:&error];
    if (error) {
        printError(@"Error applying XSL. %@", error);
        return AJCG_ErrorApplyingXSL;
    }

    // write the source file to disk
    //
    generatedCodeData = [generatedCodeData removeTrailingWhitespacesFromText];
    [generatedCodeData writeToURL:[NSURL fileURLWithPath:outputFilePath] atomically:NO];

    // STEP THREE
    //
    // prepare to generate the Objective-C header file
    //
    xslFileUrl = [NSString stringWithFormat:@"%@/%@", executableDirectory, kObjectiveCHeaderXSL];
    outputFilePath = [NSString stringWithFormat:@"%@/%@.h", outputDirectory, baseFileName];
    outputFileName = [[NSURL fileURLWithPath:outputFilePath] lastPathComponent];

    generatedCodeData = [xmlDocument objectByApplyingXSLTAtURL:[NSURL fileURLWithPath:xslFileUrl]
                                                     arguments:@{@"fileName": [NSString stringWithFormat:@"\'%@\'", outputFileName],
                                                                 @"baseFileName": [NSString stringWithFormat:@"\'%@\'", baseFileName]}
                                                         error:&error];
    if (error) {
        printError(@"Error applying XSL. %@", error);
        return AJCG_ErrorApplyingXSL;
    }

    // write the category header file to disk
    //
    generatedCodeData = [generatedCodeData removeTrailingWhitespacesFromText];
    [generatedCodeData writeToURL:[NSURL fileURLWithPath:outputFilePath] atomically:NO];

    // STEP FOUR
    //
    // prepare to generate the Objective-C source file
    //
    xslFileUrl = [NSString stringWithFormat:@"%@/%@", executableDirectory, kObjectiveCSourceXSL];
    outputFilePath = [NSString stringWithFormat:@"%@/%@.m", outputDirectory, baseFileName];
    outputFileName = [[NSURL fileURLWithPath:outputFilePath] lastPathComponent];

    generatedCodeData = [xmlDocument objectByApplyingXSLTAtURL:[NSURL fileURLWithPath:xslFileUrl]
                                                     arguments:@{@"fileName": [NSString stringWithFormat:@"\'%@\'", outputFileName],
                                                                 @"baseFileName": [NSString stringWithFormat:@"\'%@\'", baseFileName]}
                                                         error:&error];
    if (error) {
        printError(@"Error applying XSL. %@", error);
        return AJCG_ErrorApplyingXSL;
    }

    // write the category source file to disk
    //
    generatedCodeData = [generatedCodeData removeTrailingWhitespacesFromText];
    [generatedCodeData writeToURL:[NSURL fileURLWithPath:outputFilePath] atomically:NO];

    return AJCG_Ok;
}

@end