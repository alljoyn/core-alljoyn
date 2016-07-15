//
//  AJNAboutDataCheck.h
//  AllJoynFramework
//
//  Created by Jayachandra Agraharam on 12/07/2016.
//  Copyright © 2016 AllSeen Alliance. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "AJNBusAttachment.h"

/*------------------------------------------------------------------*/
#pragma mark- AJNAboutDataCheck :: Helper class to check if about data and announced data valid
/*------------------------------------------------------------------*/
@interface AJNAboutDataCheck : NSObject

- (BOOL)isDataValid:(NSDictionary*)aboutData andAnnounceData:(NSDictionary*)announceData;

@end
