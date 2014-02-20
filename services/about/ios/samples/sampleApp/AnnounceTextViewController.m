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

#import "AnnounceTextViewController.h"
#import "AJNAboutDataConverter.h"

@interface AnnounceTextViewController ()
@property (weak, nonatomic) IBOutlet UITextView *announceInformation;

@end

@implementation AnnounceTextViewController

- (NSString *)objectDescriptionsToString:(NSMutableDictionary *)qnsObjectDesc
{
	NSMutableString *qnsObjectDescContent = [[NSMutableString alloc] init];
    
	for (NSString *key in qnsObjectDesc.allKeys) {
		//  iterate over the NSMutableDictionary
		//  path: <key>
		[qnsObjectDescContent appendString:[NSString stringWithFormat:@"path: %@ \n", key]];
		[qnsObjectDescContent appendString:[NSString stringWithFormat:@"interfaces: "]];
		//  interfaces: <NSMutableArray of NSString with ' ' between each element>
		for (NSString *intrf in qnsObjectDesc[key]) {
			//  get NSString from the received object(NSMutableArray)
			[qnsObjectDescContent appendString:[NSString stringWithFormat:@"%@ ", intrf]];
		}
		[qnsObjectDescContent appendString:[NSString stringWithFormat:@"\n\n"]];
	}
    
	return (qnsObjectDescContent);
} //  parseObjectDescriptions

- (void)viewDidLoad
{
	[super viewDidLoad];
    
	//  retrive AJNAnnouncement by the  announcementButtonCurrentTitle unique name
	NSString *txt = [[NSString alloc] init];
    
	//  set title
	NSString *title = [self.ajnAnnouncement busName];
    
	txt = [txt stringByAppendingFormat:@"%@\n%@\n", title, [@"" stringByPaddingToLength :[title length] + 10 withString : @"-" startingAtIndex : 0]];
    
	//  set body
	txt = [txt stringByAppendingFormat:@"BusName: %@\n", [self.ajnAnnouncement busName]];
    
	txt = [txt stringByAppendingFormat:@"Port: %hu\n", [self.ajnAnnouncement port]];
    
    txt = [txt stringByAppendingFormat:@"Version: %u\n", [self.ajnAnnouncement version]];
    
	txt = [txt stringByAppendingString:@"\n\n"];
    
	//  set AboutMap info
	txt = [txt stringByAppendingFormat:@"About map:\n"];
    
	txt = [txt stringByAppendingString:[AJNAboutDataConverter aboutDataDictionaryToString:([self.ajnAnnouncement aboutData])]];
    
	txt = [txt stringByAppendingString:@"\n\n"];
    
	//  set ObjectDesc info
	txt = [txt stringByAppendingFormat:@"Bus Object Description:\n"];
    
	txt = [txt stringByAppendingString:[self objectDescriptionsToString:[self.ajnAnnouncement objectDescriptions]]];
    
	self.announceInformation.text = txt;
}


@end
