//
//  AJNAboutDataCheck.m
//  AllJoynFramework
//
//  Created by Jayachandra Agraharam on 12/07/2016.
//  Copyright © 2016 AllSeen Alliance. All rights reserved.
//

#import "AJNAboutDataCheck.h"


static NSArray *ABOUT_REQUIRED_FIELDS(){
    return @[@"AppId",
             @"DefaultLanguage",
             @"DeviceId",
             @"ModelNumber",
             @"SupportedLanguages",
             @"SoftwareVersion",
             @"AJSoftwareVersion",
             @"AppName",
             @"Manufacturer",
             @"Description"];
}

static NSArray *ABOUT_ANNOUNCED_FIELDS(){
    return @[@"AppId",
             @"DefaultLanguage",
             @"DeviceId",
             @"ModelNumber",
             @"DeviceName",
             @"AppName",
             @"Manufacturer"];
}

@interface AJNAboutDataCheck()
{
    BOOL isValid;
}
@end

@implementation AJNAboutDataCheck
- (id)init{
    self = [super init];
    if (self) {
        isValid = NO;
    }
    return self;
}

// aboutData is key value pair of AJNMessageArgument objects
// Are required fields all set?
- (BOOL)areRequiredFieldsSet:(NSDictionary*)aboutData{
    BOOL allSet = YES;
    for(NSString *fieldName in ABOUT_REQUIRED_FIELDS()){
        if (![aboutData.allKeys containsObject:fieldName]) {
            NSLog(@"AboutData miss required field %@",fieldName);
            allSet = NO;
            break;
        }
    }
    return allSet;
}

// announceData is key value pair of AJNMessageArgument objects
// Are announced fields all set?
- (BOOL)areAnnouncedFieldsSet:(NSDictionary*)announceData{
    BOOL allSet = YES;
    for(NSString *fieldName in ABOUT_ANNOUNCED_FIELDS()){
        if (![announceData.allKeys containsObject:fieldName]) {
            NSLog(@"AboutData miss announced field %@",fieldName);
            allSet = NO;
            break;
        }
    }
    return allSet;
}

// Is announced data and getAboutData consistent
- (BOOL)isDataConsistent:(NSDictionary*)aboutData andAnnounceData:(NSDictionary*)announceData{
    BOOL isConsistent = true;
    NSArray *announcedFields = announceData.allKeys;
    
    // AppId equals does not work
    for(NSString *announcedField in announcedFields){
        if ([announcedField isEqualToString:@"AppId"] && [aboutData.allKeys containsObject:announcedField]) {
            // Check value match
            AJNMessageArgument *announcedValue = [announceData objectForKey:announcedField];
            AJNMessageArgument *aboutValue = [aboutData objectForKey:announcedField];
            
            if (![announcedValue isEqual:aboutValue])
            {
                isConsistent = NO;
                NSLog(@" %@ mismatch: %@ vs %@",announcedField,announcedValue,aboutValue);
                break;
            }
            
        }
    }
    return isConsistent;
}

// Is data valid
- (BOOL)isDataValid:(NSDictionary*)aboutData andAnnounceData:(NSDictionary*)announceData{
    
    isValid = false;
    
    // Check if any required field is missing
    if ([self areRequiredFieldsSet:aboutData])
    {
        // Check if any announced field is missing
        if ([self areAnnouncedFieldsSet:announceData])
        {
            // Check if any mismatch between about value and announced value
            if ([self isDataValid:aboutData andAnnounceData:announceData])
            {
                isValid = true;
            }
        }
    }
    return isValid;
}


@end