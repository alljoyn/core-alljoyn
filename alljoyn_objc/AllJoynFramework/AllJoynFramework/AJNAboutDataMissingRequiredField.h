//
//  AJNAboutDataMissingRequiredField.h
//  AllJoynFramework
//
//  Created by Jayachandra Agraharam on 12/07/2016.
//  Copyright © 2016 AllSeen Alliance. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "AJNAboutDataListener.h"


#pragma mark- AJNAboutDataMissingRequiredField
/*
 * Invalid about data since required field DeviceId is missing
 */
@interface AJNAboutDataMissingRequiredField : NSObject<AJNAboutDataListener>
//both are key value pairs of string anf AJNMessageArgumemt object
@property (nonatomic, strong) NSDictionary *aboutData;
@property (nonatomic, strong) NSDictionary *announceData;
- (void)aboutDataMissingRequiredField:(NSString*)language;
@end
