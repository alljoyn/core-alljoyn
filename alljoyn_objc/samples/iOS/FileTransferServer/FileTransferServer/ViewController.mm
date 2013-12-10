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

#import "ViewController.h"
#import "AJNStatus.h"
#import "AJNMessageArgument.h"
#import "AJNServiceController.h"
#import "Constants.h"
#import "FileTransferObject.h"
#import <alljoyn/Message.h>

#define kMaximumAllJoynArrayLength 131072
NSString * const kImageFileName = @"image";

@interface ViewController () <AJNServiceDelegate>

@property (nonatomic, strong) AJNServiceController *serviceController;
@property (nonatomic, strong) FileTransferObject *fileTransferObject;

- (AJNBusObject *)objectOnBus:(AJNBusAttachment *)bus;
- (void)shouldUnloadObjectOnBus:(AJNBusAttachment *)bus;

- (void)transferFile;

@end

@implementation ViewController

@synthesize serviceController = _serviceController;
@synthesize fileTransferObject = _fileTransferObject;

- (NSString *)applicationName
{
    return kAppName;
}

- (NSString *)serviceName
{
    return kServiceName;
}

- (AJNBusNameFlag)serviceNameFlags
{
    return kAJNBusNameFlagDoNotQueue | kAJNBusNameFlagReplaceExisting;
}

- (AJNSessionPort)sessionPort
{
    return kServicePort;
}

- (AJNBusObject *)object
{
    return self.fileTransferObject;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view, typically from a nib.

    // allocate the service bus controller and set bus properties
    //
    self.serviceController = [[AJNServiceController alloc] init];
    self.serviceController.trafficType = kAJNTrafficMessages;
    self.serviceController.proximityOptions = kAJNProximityAny;
    self.serviceController.transportMask = kAJNTransportMaskAny;
    self.serviceController.allowRemoteMessages = YES;
    self.serviceController.multiPointSessionsEnabled = NO;
    self.serviceController.delegate = self;
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)transferFile
{
    dispatch_queue_t fileTransferQueue = dispatch_queue_create("org.alljoyn.samples.fileTransferQueue", NULL);
    
    dispatch_async(fileTransferQueue, ^{
        NSString *filePath = [[NSBundle mainBundle] pathForResource:@"image" ofType:@"png"];
        NSFileHandle *fileHandle = [NSFileHandle fileHandleForReadingAtPath:filePath];
        NSDictionary *fileAttributes = [[NSFileManager defaultManager] attributesOfItemAtPath:filePath error:nil];
        unsigned long long totalFileSize = [fileAttributes fileSize];
        

        const uint8_t flags = 0x20;
        
        NSInteger bytesRead = 0;
        NSInteger totalBytesRead = 0;
        NSInteger countOfChunksSent = 0;
        NSData *fileData;
        uint8_t buffer[kMaximumAllJoynArrayLength];
        ajn::MsgArg msgArg;
    
        do {
            fileData = [fileHandle readDataOfLength:kMaximumAllJoynArrayLength];
            bytesRead = fileData.length;
            [fileData getBytes:buffer length:kMaximumAllJoynArrayLength];
            totalBytesRead += bytesRead;
            countOfChunksSent++;
            
            msgArg.Set("ay", bytesRead, buffer);

            [self.fileTransferObject sendTransferFileNamed:filePath currentIndex:[NSNumber numberWithInteger:countOfChunksSent] fileData:[[AJNMessageArgument alloc] initWithHandle:&msgArg] inSession:self.serviceController.sessionId toDestination:nil flags:flags];
            
            dispatch_async(dispatch_get_main_queue(), ^{
                self.progressView.progress = (float)totalBytesRead / (float)totalFileSize;
            });
        }
        while (totalBytesRead < totalFileSize);
        
        if (bytesRead != 0) {
            msgArg.Set("ay", 0, NULL);
            [self.fileTransferObject sendTransferFileNamed:filePath currentIndex:[NSNumber numberWithInteger:countOfChunksSent] fileData:[[AJNMessageArgument alloc] initWithHandle:&msgArg] inSession:self.serviceController.sessionId toDestination:nil flags:flags];
            
        }
        
    });
    
}

- (IBAction)didTouchStartButton:(id)sender
{
    self.eventsTextView.text = nil;
    
    if (self.serviceController.bus.isStarted) {
        [self.serviceController stop];
        [self.startButton setTitle:@"Start" forState:UIControlStateNormal];
    }
    else {
        [self.serviceController start];
        [self.startButton setTitle:@"Stop" forState:UIControlStateNormal];
    }    
}


- (void)shouldUnloadObjectOnBus:(AJNBusAttachment *)bus
{
    self.fileTransferObject = nil;
    [self.startButton setTitle:@"Start" forState:UIControlStateNormal];
}

- (AJNBusObject *)objectOnBus:(AJNBusAttachment *)bus
{
    self.fileTransferObject = [[FileTransferObject alloc] initWithBusAttachment:self.serviceController.bus onPath:kServicePath];
    
    return self.fileTransferObject;
}

- (void)didJoin:(NSString *)joiner inSessionWithId:(AJNSessionId)sessionId onSessionPort:(AJNSessionPort)sessionPort
{
    [self.serviceController.bus enableConcurrentCallbacks];
    // start the file transfer once the client joins the session
    //
    [self transferFile];
}

- (void)didReceiveStatusMessage:(NSString*)message
{
    dispatch_async(dispatch_get_main_queue(), ^{
        NSMutableString *string = self.eventsTextView.text.length ? [self.eventsTextView.text mutableCopy] : [[NSMutableString alloc] init];
        NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
        [formatter setTimeStyle:NSDateFormatterMediumStyle];
        [formatter setDateStyle:NSDateFormatterShortStyle];
        [string appendFormat:@"[%@]\n",[formatter stringFromDate:[NSDate date]]];
        [string appendString:message];
        [string appendString:@"\n\n"];
        [self.eventsTextView setText:string];
        NSLog(@"%@",message);
        [self.eventsTextView scrollRangeToVisible:NSMakeRange([self.eventsTextView.text length], 0)];
    });
}

@end
