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