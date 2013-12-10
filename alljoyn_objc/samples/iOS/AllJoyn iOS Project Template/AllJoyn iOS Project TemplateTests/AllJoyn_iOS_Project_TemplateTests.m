////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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

#import "AllJoyn_iOS_Project_TemplateTests.h"
#import "SampleClient.h"
#import "SampleService.h"

@interface AllJoyn_iOS_Project_TemplateTests() <SampleClientDelegate>

@property (nonatomic, strong) SampleClient *sampleClient;
@property (nonatomic, strong) SampleService *sampleService;
@property (nonatomic) BOOL clientFoundService;

@end

@implementation AllJoyn_iOS_Project_TemplateTests

@synthesize sampleClient = _sampleClient;
@synthesize sampleService = _sampleService;
@synthesize clientFoundService = _clientFoundService;

- (void)setUp
{
    [super setUp];
    
    // Set-up code here.
    //
    self.clientFoundService = NO;
    
    self.sampleClient = [[SampleClient alloc] initWithDelegate:self];
    self.sampleService = [[SampleService alloc] init];
    
    [self.sampleService start];
    [self.sampleClient start];
}

- (void)tearDown
{
    // Tear-down code here.
    //
    [self.sampleClient stop];
    [self.sampleService stop];
    self.clientFoundService = NO;
    
    [super tearDown];
}

- (void)testClientShouldBeAbleToConcatenateStringsUsingService
{
    // wait a bit until the client connects to the service
    //
    STAssertTrue([self waitForCompletion:15 onFlag:&_clientFoundService], @"The client failed to connect to the service in a timely manner.");
    
    // call the service and ask it to concatenate the strings together, then
    // verify the result of the method call.
    //
    NSString *concatenatedString = [self.sampleClient callServiceToConcatenateString:@"Hello" withString:@" AllJoyn World!!!!!!"];
    STAssertTrue(concatenatedString != nil, @"The returned string was nil");
    STAssertTrue([concatenatedString compare:@"Hello AllJoyn World!!!!!!"] == NSOrderedSame, @"The string returned to the client <%@> was not correctly concatenated by the service.", concatenatedString);
    
}

#pragma mark - Asynchronous test case support

- (BOOL)waitForCompletion:(NSTimeInterval)timeoutSeconds onFlag:(BOOL*)flag
{
    NSDate *timeoutDate = [NSDate dateWithTimeIntervalSinceNow:timeoutSeconds];
    
    do {
        [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:timeoutDate];
        if ([timeoutDate timeIntervalSinceNow] < 0.0) {
            break;
        }
    } while (!*flag);
    
    return *flag;
}

#pragma mark - SampleClientDelegate implementation

- (void)didConnectWithService
{
    self.clientFoundService = YES;
}

@end
