/******************************************************************************
 * Copyright (c) 2013-2014, AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/
//

#import <SystemConfiguration/CaptiveNetwork.h>
#import "qcc/String.h"
#import "GetIconCallViewController.h"
#import "AJNAboutIconClient.h"
#import "IconUrlViewController.h"

@interface GetIconCallViewController ()

@property (weak, nonatomic) IBOutlet UILabel *lblVersion;
@property (weak, nonatomic) IBOutlet UILabel *lblMimeType;
@property (weak, nonatomic) IBOutlet UILabel *lblSize;
@property (weak, nonatomic) IBOutlet UIImageView *iconView;

@property (nonatomic) NSString *currLanguage;
@property (nonatomic) AJNAboutIconClient *ajnAboutIconClient;
@property (nonatomic) AJNSessionId sessionID;

@property (weak, nonatomic) IBOutlet UIButton *btnIconUrl;
@property (nonatomic) NSString *btnIconUrlTitle;
@property (nonatomic) UIAlertView *alertRefresh;
@property (nonatomic) UIAlertView *alertNoSession;

@end

@implementation GetIconCallViewController

- (void)viewDidLoad
{
	[super viewDidLoad];
	[self prepareAlerts];
	[self setDefaultGui];
}

- (void)prepareAlerts
{
	self.alertRefresh = [[UIAlertView alloc] initWithTitle:@"Choose option:" message:@"" delegate:self cancelButtonTitle:@"Cancel" otherButtonTitles:@"Refresh", nil];
	self.alertRefresh.alertViewStyle = UIAlertViewStyleDefault;
	self.alertRefresh.tag = 1;
    
	self.alertNoSession = [[UIAlertView alloc] initWithTitle:@"Error" message:@"Session is not connected, check the connection and reconnect." delegate:self cancelButtonTitle:@"OK" otherButtonTitles:nil];
	self.alertNoSession.alertViewStyle = UIAlertViewStyleDefault;
	self.alertNoSession.tag = 2;
}

- (void)setDefaultGui
{
	self.btnIconUrlTitle = @"";
    
	//  create about client
	self.ajnAboutIconClient = [[AJNAboutIconClient alloc] initWithBus:self.clientBusAttachment];
    
	//  create sessionOptions
	AJNSessionOptions *opt1 = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:false proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
    
	//  call joinSession
	self.sessionID = [self.clientBusAttachment joinSessionWithName:[self.ajnAnnouncement busName] onPort:[self.ajnAnnouncement port] withDelegate:(nil) options:opt1];
    
	if (self.sessionID == 0 || self.sessionID == -1) {
        
         NSLog(@"[%@] [%@] Failed to join session. sid=%u", @"ERROR", [[self class] description],self.sessionID);

        [self.alertNoSession show];
		return;
	}
	//  display icon data
	[self getIconData];
}

//  display alert
- (IBAction)didTouchMoreInfo:(id)sender
{
	[self.alertRefresh show];
}

//  get the user's input from the alert dialog
- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
	switch (alertView.tag) {
		case 1: //alertRefresh
		{
			if (buttonIndex == 1) { //user pressed Refresh
				[self getIconData];
			}
			else {
				// cancel
			}
		}
            break;
            
		case 2: //alertNoSession
		{
		}
            break;
            
		default:
		{
            NSLog(@"[%@] [%@] alertView.tag is wrong", @"ERROR", [[self class] description]);
		}
            break;
	}
}

- (void)getIconData
{
     NSLog(@"[%@] [%@] Display icon data", @"DEBUG", [[self class] description]);

    QStatus status;
    
	//  Icon Data
	NSString *announcementBusName = [self.ajnAnnouncement busName];
    
	if (self.ajnAboutIconClient) {
		//  version
		int version;
		status = [self.ajnAboutIconClient versionFromBusName:announcementBusName version:version sessionId:self.sessionID];
		if (status == ER_OK) {
			self.lblVersion.text = [NSString stringWithFormat:@"%d", version];
		}
		//  mimeType
		NSString *mimeType;
		status = [self.ajnAboutIconClient mimeTypeFromBusName:announcementBusName mimeType:&mimeType sessionId:self.sessionID];
		self.lblMimeType.text = (status == ER_OK && [mimeType length]) ? mimeType : @"";
		//  size
		size_t contentSize;
		status = [self.ajnAboutIconClient sizeFromBusName:announcementBusName size:contentSize sessionId:self.sessionID];
		self.lblSize.text = (status == ER_OK && contentSize) ? [NSString stringWithFormat:@"%zu", contentSize] : @"";
		//  url
		NSString *url;
		status = [self.ajnAboutIconClient urlFromBusName:announcementBusName url:&url sessionId:self.sessionID];
		self.btnIconUrlTitle = (status == ER_OK && [url length]) ? url : @"";
		[self.btnIconUrl setTitle:self.btnIconUrlTitle forState:UIControlStateNormal];
        
		//  content
         NSLog(@"[%@] [%@] contentSize:%lu", @"DEBUG", [[self class] description],contentSize);

        
		if (contentSize) {
			uint8_t *content;
			status = [self.ajnAboutIconClient contentFromBusName:announcementBusName content:&content contentSize:contentSize sessionId:self.sessionID];
            
			if (status != ER_OK) {
                NSLog(@"[%@] [%@] Failed to get image content", @"ERROR", [[self class] description]);
			}
			else if (!content) {
                NSLog(@"[%@] [%@] Image content is empty", @"ERROR", [[self class] description]);
			}
			else {
				UIImage *retrievedImg = [UIImage imageWithData:[NSMutableData dataWithBytes:content length:(NSInteger)contentSize]];
				if (!retrievedImg) {
                    NSLog(@"[%@] [%@] Failed to retrive image content", @"ERROR", [[self class] description]);

				}
				else {
					self.iconView.image = retrievedImg;
                    
                    NSLog(@"[%@] [%@] Successfuly retrived image content", @"ERROR", [[self class] description]);
				}
			}
		}
	}
}

//  open IconUrlViewController when touching the url button
- (void)prepareForSegue:(UIStoryboardSegue *)segue
                 sender:(id)sender
{
	if ([segue.destinationViewController isKindOfClass:[IconUrlViewController class]]) {
		IconUrlViewController *getIconUrlView = segue.destinationViewController;
		getIconUrlView.url = self.btnIconUrlTitle;
	}
}

- (void)viewWillDisappear:(BOOL)animated
{
	[self.clientBusAttachment leaveSession:self.sessionID];
    
	[super viewWillDisappear:animated];
}

@end
