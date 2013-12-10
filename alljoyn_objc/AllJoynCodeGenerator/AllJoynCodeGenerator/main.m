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

#import <Foundation/Foundation.h>

// constants
//
NSString * const kImplementationHeaderXSL = @"objcHeader.xsl";
NSString * const kImplementationSourceXSL = @"objcSource.xsl";
NSString * const kObjectiveCHeaderXSL = @"objcExtensionHeader.xsl";
NSString * const kObjectiveCSourceXSL = @"objcExtensionSource.xsl";

const NSInteger kExpectedArgumentCount = 2;

// print the expected arguments
//
void usage(void)
{
    NSLog(@"Usage: AllJoynCodeGenerator [input file] [base file name]");
    NSLog(@"Arguments:");
    NSLog(@"   input file            = The path to the input XML file containing the DBus-formatted object model");
    NSLog(@"   base file name        = The base file name used for the generated Objective-C files");
    NSLog(@"");
    NSLog(@"The files containing the generated Objective-C source code will reside in the same directory as the input file specified.");
    NSLog(@"The xsl files used to generate the Objective-C source code should reside in the same directory as the AllJoynCodeGenerator executable.");
}


int main(int argc, const char * argv[])
{
    @autoreleasepool {
        
        //  Validate the there are the correct number of command line arguments.
        //  Remember that argv/argc includes the executable's path plus the
        //  arguments passed to it.
        //
        if (argc <= kExpectedArgumentCount) {
            
            // Print usage if not enough command line arguments.
            //
            usage();
            
            return 1;
        }
        
        //  Validate that the input file exists.
        //
        NSString *xmlFilePath = [NSString stringWithCString:argv[1] encoding:NSUTF8StringEncoding];
        if (![[NSFileManager defaultManager] fileExistsAtPath:xmlFilePath]) {
            NSLog(@"The input file specified does not exist %@", xmlFilePath);
            return 1;
        }
        
        // Validate that the base file name uses valid characters
        //
        NSString *baseFileName = [NSString stringWithCString:argv[2] encoding:NSUTF8StringEncoding];
        if (![[baseFileName stringByTrimmingCharactersInSet:[NSCharacterSet decimalDigitCharacterSet]] isEqualToString:baseFileName]
            && ![[baseFileName stringByTrimmingCharactersInSet:[NSCharacterSet letterCharacterSet]] isEqualToString:baseFileName]) {
            NSLog(@"The base file name should contain only letters or numbers.");
            return 1;
        }
        
        // Parse out the directory of the input file for use as the location of the output files
        //
        NSURL *fileUrl = [NSURL fileURLWithPath:xmlFilePath];
        NSURL *executableFileUrl = [NSURL fileURLWithPath:[NSString stringWithCString:argv[0] encoding:NSUTF8StringEncoding]];
        NSString *executableDirectory =  [[executableFileUrl URLByDeletingLastPathComponent] path];
        NSString *outputDirectory = [[fileUrl URLByDeletingLastPathComponent] path];
        NSError *error = nil;
        NSString *xslFileUrl = [NSString stringWithFormat:@"%@/%@", executableDirectory, kImplementationHeaderXSL];
        NSString *outputFilePath = [NSString stringWithFormat:@"%@/AJN%@.h", outputDirectory, baseFileName];
        NSString *outputFileName = [[NSURL fileURLWithPath:outputFilePath] lastPathComponent];

        // STEP ONE
        //
        // prepare to generate the Objective-C scaffold implementation header file
        //
        NSXMLDocument *xmlDocument = [[NSXMLDocument alloc] initWithContentsOfURL:fileUrl options:NSDataReadingMappedIfSafe error:&error];
        if (error) {
            NSLog(@"Error loading XML. %@",error);
            return 1;
        }
        
        NSData *generatedCodeData = [xmlDocument objectByApplyingXSLTAtURL:[NSURL fileURLWithPath:xslFileUrl] arguments:[NSDictionary dictionaryWithObjects:[NSArray arrayWithObject:[NSString stringWithFormat:@"\'%@\'", outputFileName]] forKeys:[NSArray arrayWithObject:@"fileName"]] error:&error];
        if (error) {
            NSLog(@"Error applying XSL. %@",error);
            return 1;
        }
        
        // write the Objective-C scaffold header file to disk
        //
        [generatedCodeData writeToURL:[NSURL fileURLWithPath:outputFilePath] atomically:YES];
        
        // STEP TWO
        //
        // prepare to generate the Objective-C++ scaffold implementation source file
        //
        xslFileUrl = [NSString stringWithFormat:@"%@/%@", executableDirectory, kImplementationSourceXSL];
        outputFilePath = [NSString stringWithFormat:@"%@/AJN%@.mm", outputDirectory, baseFileName];
        outputFileName = [[NSURL fileURLWithPath:outputFilePath] lastPathComponent];
        
        xmlDocument = [[NSXMLDocument alloc] initWithContentsOfURL:fileUrl options:NSDataReadingMappedIfSafe error:&error];
        if (error) {
            NSLog(@"Error loading XML. %@",error);
            return 1;
        }
        
        generatedCodeData = [xmlDocument objectByApplyingXSLTAtURL:[NSURL fileURLWithPath:xslFileUrl] arguments:[NSDictionary dictionaryWithObjects:[NSArray arrayWithObjects:[NSString stringWithFormat:@"\'%@\'",outputFileName], [NSString stringWithFormat:@"\'%@\'",baseFileName],nil] forKeys:[NSArray arrayWithObjects:@"fileName",@"baseFileName",nil]] error:&error];
        if (error) {
            NSLog(@"Error applying XSL. %@",error);
            return 1;
        }
        
        // write the source file to disk
        //
        [generatedCodeData writeToURL:[NSURL fileURLWithPath:outputFilePath] atomically:YES];
        
        // STEP THREE
        //
        // prepare to generate the Objective-C header file
        //
        xslFileUrl = [NSString stringWithFormat:@"%@/%@", executableDirectory, kObjectiveCHeaderXSL];
        outputFilePath = [NSString stringWithFormat:@"%@/%@.h", outputDirectory, baseFileName];
        outputFileName = [[NSURL fileURLWithPath:outputFilePath] lastPathComponent];

        xmlDocument = [[NSXMLDocument alloc] initWithContentsOfURL:fileUrl options:NSDataReadingMappedIfSafe error:&error];
        if (error) {
            NSLog(@"Error loading XML. %@",error);
            return 1;
        }
        
        generatedCodeData = [xmlDocument objectByApplyingXSLTAtURL:[NSURL fileURLWithPath:xslFileUrl] arguments:[NSDictionary dictionaryWithObjects:[NSArray arrayWithObjects:[NSString stringWithFormat:@"\'%@\'",outputFileName], [NSString stringWithFormat:@"\'%@\'",baseFileName],nil] forKeys:[NSArray arrayWithObjects:@"fileName",@"baseFileName",nil]] error:&error];
        if (error) {
            NSLog(@"Error applying XSL. %@",error);
            return 1;
        }
        
        // write the category header file to disk
        //
        [generatedCodeData writeToURL:[NSURL fileURLWithPath:outputFilePath] atomically:YES];
        
        // STEP FOUR
        //
        // prepare to generate the Objective-C source file
        //
        xslFileUrl = [NSString stringWithFormat:@"%@/%@", executableDirectory, kObjectiveCSourceXSL];
        outputFilePath = [NSString stringWithFormat:@"%@/%@.m", outputDirectory, baseFileName];
        outputFileName = [[NSURL fileURLWithPath:outputFilePath] lastPathComponent];
        
        xmlDocument = [[NSXMLDocument alloc] initWithContentsOfURL:fileUrl options:NSDataReadingMappedIfSafe error:&error];
        if (error) {
            NSLog(@"Error loading XML. %@",error);
            return 1;
        }
        
        generatedCodeData = [xmlDocument objectByApplyingXSLTAtURL:[NSURL fileURLWithPath:xslFileUrl] arguments:[NSDictionary dictionaryWithObjects:[NSArray arrayWithObjects:[NSString stringWithFormat:@"\'%@\'",outputFileName], [NSString stringWithFormat:@"\'%@\'",baseFileName],nil] forKeys:[NSArray arrayWithObjects:@"fileName",@"baseFileName",nil]] error:&error];
        if (error) {
            NSLog(@"Error applying XSL. %@",error);
            return 1;
        }
        
        // write the category source file to disk
        //
        [generatedCodeData writeToURL:[NSURL fileURLWithPath:outputFilePath] atomically:YES];
    }
    return 0;
}

